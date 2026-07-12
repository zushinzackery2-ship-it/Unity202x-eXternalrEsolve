#include <er2/unity2/dumpsdk/writers/cli/UnityCliMetadataBuilder.h>

namespace er2::UnityCli
{
bool MetadataBuilder::Build(const CollectedAssembly& assembly, std::vector<uint8_t>& metadata, std::vector<uint8_t>& methodBodies)
{
InitializeAssembly(assembly);
RegisterTypeDefinitions(assembly);
PopulateTypeMembers(assembly);
methodBodies = methodBodies_.Data();
metadata = BuildMetadataRoot(rows_, heaps_);
return !metadata.empty();
}

void MetadataBuilder::InitializeAssembly(const CollectedAssembly& assembly)
{
const std::string baseName = StripExtension(!assembly.name.empty() ? assembly.name : assembly.fileName);
rows_.ModuleName = heaps_.GetString(baseName + ".dll");
rows_.ModuleMvid = heaps_.AddGuid(BuildGuid(baseName));
rows_.Assembly.Name = heaps_.GetString(baseName);

AssemblyRefRow coreLib;
coreLib.PublicKeyOrToken = heaps_.AddBlob({ 0xB7, 0x7A, 0x5C, 0x56, 0x19, 0x34, 0xE0, 0x89 });
coreLib.Name = heaps_.GetString("mscorlib");
rows_.AssemblyRefs.push_back(coreLib);

objectRef_ = AddTypeRef("System", "Object");
valueTypeRef_ = AddTypeRef("System", "ValueType");
enumRef_ = AddTypeRef("System", "Enum");
delegateRef_ = AddTypeRef("System", "MulticastDelegate");

TypeDefRow moduleType;
moduleType.Name = heaps_.GetString("<Module>");
rows_.TypeDefs.push_back(moduleType);
}

std::array<uint8_t, 16> MetadataBuilder::BuildGuid(const std::string& value)
{
uint64_t hash = 1469598103934665603ull;
for (char c : value)
{
hash ^= static_cast<uint8_t>(c);
hash *= 1099511628211ull;
}

std::array<uint8_t, 16> guid{};
for (size_t i = 0; i < 8; ++i)
{
guid[i] = static_cast<uint8_t>(hash >> (i * 8));
guid[i + 8] = static_cast<uint8_t>((hash ^ 0xA5A5A5A5A5A5A5A5ull) >> (i * 8));
}
return guid;
}

uint32_t MetadataBuilder::AddTypeRef(const std::string& namespaceName, const std::string& typeName)
{
const std::string key = namespaceName + "." + typeName;
const auto existing = typeRefs_.find(key);
if (existing != typeRefs_.end())
return existing->second;

TypeRefRow row;
row.ResolutionScope = EncodeResolutionScope(1, TableId::AssemblyRef);
row.Name = heaps_.GetString(typeName);
row.Namespace = heaps_.GetString(namespaceName);
rows_.TypeRefs.push_back(row);

const uint32_t rowIndex = static_cast<uint32_t>(rows_.TypeRefs.size());
typeRefs_.emplace(key, rowIndex);
return rowIndex;
}

void MetadataBuilder::RegisterTypeDefinitions(const CollectedAssembly& assembly)
{
for (size_t i = 0; i < assembly.types.size(); ++i)
{
const CollectedType& type = assembly.types[i];
TypeDefRow row;
const std::string name = CleanMetadataName(type.name, "Type_" + std::to_string(i));
row.Flags = BuildTypeFlags(type);
row.Name = heaps_.GetString(name);
row.Namespace = heaps_.GetString(type.namespaceName);
rows_.TypeDefs.push_back(row);

const uint32_t rowIndex = static_cast<uint32_t>(rows_.TypeDefs.size());
RegisterLocalType(name, type.namespaceName, rowIndex, type.kind);
}
}

void MetadataBuilder::RegisterLocalType(const std::string& name, const std::string& namespaceName, uint32_t rowIndex, TypeKind kind)
{
LocalTypeInfo info{ rowIndex, kind };
localTypes_.try_emplace(name, info);
if (!namespaceName.empty())
localTypes_.try_emplace(namespaceName + "." + name, info);
}

void MetadataBuilder::PopulateTypeMembers(const CollectedAssembly& assembly)
{
for (size_t i = 0; i < assembly.types.size(); ++i)
{
const CollectedType& type = assembly.types[i];
TypeDefRow& row = rows_.TypeDefs[i + 1];
row.Extends = BuildExtends(type);
row.FieldList = static_cast<uint32_t>(rows_.Fields.size()) + 1;
AddFields(type);
row.MethodList = static_cast<uint32_t>(rows_.Methods.size()) + 1;
std::unordered_map<std::string, uint32_t> methodRows = AddMethods(type);
AddProperties(static_cast<uint32_t>(i + 2), type, methodRows);
}
}

uint32_t MetadataBuilder::BuildExtends(const CollectedType& type)
{
if (type.kind == TypeKind::Interface)
return 0;
if (type.kind == TypeKind::Struct)
return EncodeTypeDefOrRef(valueTypeRef_, TableId::TypeRef);
if (type.kind == TypeKind::Enum)
return EncodeTypeDefOrRef(enumRef_, TableId::TypeRef);
if (type.kind == TypeKind::Delegate)
return EncodeTypeDefOrRef(delegateRef_, TableId::TypeRef);
if (!type.parentName.empty())
{
if (const LocalTypeInfo* parent = FindLocalType(type.parentName))
return EncodeTypeDefOrRef(parent->RowIndex, TableId::TypeDef);
}
return EncodeTypeDefOrRef(objectRef_, TableId::TypeRef);
}

bool BuildMetadata(const CollectedAssembly& assembly, std::vector<uint8_t>& metadata, std::vector<uint8_t>& methodBodies)
{
MetadataBuilder builder;
return builder.Build(assembly, metadata, methodBodies);
}
}