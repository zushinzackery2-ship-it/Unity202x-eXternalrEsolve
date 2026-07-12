#include <er2/unity2/dumpsdk/offline/OfflineCollector.h>

#include <er2/os/win/local_memory_accessor.hpp>
#include <er2/unity2/dumpsdk/collected_data.hpp>
#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/offline/Il2CppStructs.h>
#include <er2/unity2/dumpsdk/offline/Metadata.h>
#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>
#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>
#include <er2/unity2/metadata.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <unordered_map>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

const char* kPrimitiveTypeNames[] = {
    "END",
    "void",
    "bool",
    "char",
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "int64_t",
    "uint64_t",
    "float",
    "double",
    "string",
    "PTR",
    "BYREF",
    "VALUETYPE",
    "CLASS",
    "VAR",
    "ARRAY",
    "GENERICINST",
    "TYPEDBYREF",
    "unused",
    "unused",
    "unused",
    "unused",
    "unused",
    "Object",
    "SZARRAY",
    "MVAR",
};

int CountMethodsForSearch(const Metadata& metadata)
{
    if (metadata.Version() <= 24.1)
    {
        int count = 0;
        for (const auto& method : metadata.MethodDefs())
        {
            if (method.methodIndex >= 0)
            {
                ++count;
            }
        }
        return count;
    }
    return static_cast<int>(metadata.MethodDefs().size());
}

int64_t EstimateMetadataUsagesCount(const Metadata& metadata)
{
    const auto& header = metadata.Header();
    if (metadata.Version() > 16.0 && metadata.Version() < 27.0)
    {
        return header.metadataUsagePairsCount;
    }
    return 0;
}

bool TryFindRegistrationsWithEr2(
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    uintptr_t metaBase,
    uintptr_t& codeRegistration,
    uintptr_t& metadataRegistration)
{
    codeRegistration = 0;
    metadataRegistration = 0;
    if (moduleBase == 0 || moduleSize == 0 || metaBase == 0)
    {
        return false;
    }

    LocalMemoryAccessor mem;
    Il2CppRegs regs{};
    const bool found = FindIl2CppRegistrations(
        mem,
        moduleBase,
        moduleSize,
        {},
        metaBase,
        0x200000u,
        45.0,
        regs);

    codeRegistration = regs.codeRegistration;
    metadataRegistration = regs.metadataRegistration;

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] er2 registrations found={} cr=0x{:X} mr=0x{:X}",
            found,
            codeRegistration,
            metadataRegistration));

    return codeRegistration != 0 && metadataRegistration != 0;
}

std::string AccessFromFieldFlags(uint32_t flags)
{
    switch (flags & kFieldAccessMask)
    {
    case 1: return "private";
    case 2: return "public";
    case 3: return "family";
    case 4: return "assembly";
    case 5: return "fam-and-assem";
    case 6: return "fam-or-assem";
    default: return "private";
    }
}

std::string AccessFromMethodFlags(uint16_t flags)
{
    switch (flags & kMethodMemberAccessMask)
    {
    case 1: return "private";
    case 2: return "public";
    case 3: return "family";
    case 4: return "assembly";
    case 5: return "fam-and-assem";
    case 6: return "fam-or-assem";
    default: return "private";
    }
}

TypeKind ResolveTypeKind(const Il2CppTypeDefinition& typeDef)
{
    if (typeDef.IsEnum())
    {
        return TypeKind::Enum;
    }
    if (typeDef.IsValueType())
    {
        return TypeKind::Struct;
    }
    return TypeKind::Class;
}

class TypeNameResolver
{
public:
    TypeNameResolver(const Metadata& metadata, const PeImage& pe, const RegistrationInitResult& registration)
        : metadata_(metadata)
        , pe_(pe)
        , registration_(registration)
    {
    }

    std::string ResolveByTypeIndex(int typeIndex, bool addNamespace = true)
    {
        return ResolveByTypeIndex(typeIndex, addNamespace, false);
    }

    std::string ResolveTypeDefName(const Il2CppTypeDefinition& typeDef, bool addNamespace = true)
    {
        std::string prefix;
        if (typeDef.declaringTypeIndex != -1)
        {
            prefix = ResolveByTypeIndex(typeDef.declaringTypeIndex, addNamespace, true) + ".";
        }
        else if (addNamespace)
        {
            const std::string ns = metadata_.GetStringFromIndex(typeDef.namespaceIndex);
            if (!ns.empty())
            {
                prefix = ns + ".";
            }
        }
        std::string typeName = metadata_.GetStringFromIndex(typeDef.nameIndex);
        const size_t tick = typeName.find('`');
        if (tick != std::string::npos)
        {
            typeName = typeName.substr(0, tick);
        }
        return prefix + typeName;
    }

private:
    std::string ResolveByTypeIndex(int typeIndex, bool addNamespace, bool isNested)
    {
        if (typeIndex < 0 || typeIndex >= static_cast<int>(registration_.types.size()))
        {
            return "unknown";
        }
        return ResolveRuntimeType(registration_.types[static_cast<size_t>(typeIndex)], addNamespace, isNested);
    }

    std::string ResolveRuntimeType(const Il2CppTypeRuntime& runtimeType, bool addNamespace, bool isNested)
    {
        switch (runtimeType.type)
        {
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
        {
            const auto it = registration_.typePtrToIndex.find(static_cast<uintptr_t>(runtimeType.NestedType()));
            if (it != registration_.typePtrToIndex.end())
            {
                return ResolveRuntimeType(registration_.types[it->second], addNamespace, false) + "*";
            }
            return "void*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
        {
            const auto it = registration_.typePtrToIndex.find(static_cast<uintptr_t>(runtimeType.NestedType()));
            if (it != registration_.typePtrToIndex.end())
            {
                return ResolveRuntimeType(registration_.types[it->second], addNamespace, false) + "[]";
            }
            return "unknown[]";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST:
        {
            if (runtimeType.type == Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST)
            {
                Il2CppGenericClass genericClass{};
                if (!ReadAbs(pe_, runtimeType.GenericClass(), &genericClass, sizeof(genericClass)))
                {
                    return "unknown";
                }
                const int64_t typeDefIndex = metadata_.Version() >= 27.0
                    ? static_cast<int64_t>(genericClass.type)
                    : genericClass.typeDefinitionIndex;
                if (typeDefIndex < 0 || typeDefIndex >= static_cast<int64_t>(metadata_.TypeDefs().size()))
                {
                    return "unknown";
                }
                return ResolveTypeDefName(metadata_.TypeDefs()[static_cast<size_t>(typeDefIndex)], addNamespace);
            }

            const int64_t klassIndex = runtimeType.KlassIndex();
            if (klassIndex < 0 || klassIndex >= static_cast<int64_t>(metadata_.TypeDefs().size()))
            {
                return "unknown";
            }
            return ResolveTypeDefName(metadata_.TypeDefs()[static_cast<size_t>(klassIndex)], addNamespace);
        }
        default:
        {
            const size_t index = static_cast<size_t>(runtimeType.type);
            if (index < sizeof(kPrimitiveTypeNames) / sizeof(kPrimitiveTypeNames[0]))
            {
                return kPrimitiveTypeNames[index];
            }
            return "unknown";
        }
        }
    }

    const Metadata& metadata_;
    const PeImage& pe_;
    const RegistrationInitResult& registration_;
};

uintptr_t GetMethodPointer(const RegistrationInitResult& registration,
    double version,
    const std::string& imageName,
    const Il2CppMethodDefinition& methodDef)
{
    if (version >= 24.2)
    {
        const auto it = registration.codeGenModuleMethodPointers.find(imageName);
        if (it == registration.codeGenModuleMethodPointers.end())
        {
            return 0;
        }
        const uint32_t methodPointerIndex = methodDef.token & 0x00FFFFFFu;
        if (methodPointerIndex == 0)
        {
            return 0;
        }
        const int idx = static_cast<int>(methodPointerIndex) - 1;
        if (idx < 0 || idx >= static_cast<int>(it->second.size()))
        {
            return 0;
        }
        return it->second[static_cast<size_t>(idx)];
    }
    if (methodDef.methodIndex >= 0 &&
        methodDef.methodIndex < static_cast<int32_t>(registration.legacyMethodPointers.size()))
    {
        return registration.legacyMethodPointers[static_cast<size_t>(methodDef.methodIndex)];
    }
    return 0;
}

size_t GetFieldOffset(const PeImage& pe,
    const RegistrationInitResult& registration,
    int typeIndex,
    int fieldIndexInType,
    size_t flatFieldIndex,
    bool isValueType,
    bool isStatic)
{
    if (registration.fieldOffsets.empty())
    {
        return 0;
    }

    int32_t offset = -1;
    if (registration.fieldOffsetsArePointers)
    {
        if (typeIndex < 0 || typeIndex >= static_cast<int>(registration.fieldOffsets.size()))
        {
            return 0;
        }
        const uint64_t ptr = registration.fieldOffsets[static_cast<size_t>(typeIndex)];
        if (ptr == 0)
        {
            return 0;
        }
        ReadAbs(pe, static_cast<uint64_t>(ptr + static_cast<uint64_t>(fieldIndexInType) * 4ull), &offset, sizeof(offset));
    }
    else
    {
        if (flatFieldIndex >= registration.fieldOffsets.size())
        {
            return 0;
        }
        offset = static_cast<int32_t>(registration.fieldOffsets[flatFieldIndex]);
    }

    if (offset > 0 && isValueType && !isStatic)
    {
        offset -= 16;
    }
    return offset > 0 ? static_cast<size_t>(offset) : 0;
}

} // namespace

bool Collect(
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const uint8_t* metaBytes,
    size_t metaSize,
    uintptr_t metaBase,
    CollectedData& out,
    std::string& error,
    RegistrationInitResult* registrationOut)
{
    out = {};
    if (moduleBase == 0 || moduleSize == 0 || metaBytes == nullptr || metaSize == 0)
    {
        error = "Invalid Collect input";
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Collect: snapshot+parse PE");
    PeImage pe{};
    if (!LoadPeImageFromModuleRange(moduleBase, moduleSize, pe, error))
    {
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Collect: parse metadata");
    Metadata metadata;
    try
    {
        metadata.Load(metaBytes, metaSize);
    }
    catch (const std::exception& ex)
    {
        error = std::string("Metadata parse failed: ") + ex.what();
        return false;
    }

    RegistrationSearchInput searchInput{};
    searchInput.methodCount = CountMethodsForSearch(metadata);
    searchInput.typeDefCount = static_cast<int>(metadata.TypeDefs().size());
    searchInput.imageCount = static_cast<int>(metadata.ImageDefs().size());
    searchInput.metadataUsagesCount = EstimateMetadataUsagesCount(metadata);
    searchInput.version = metadata.Version();

    DumpSdkLog(DumpSdkLogLevel::Info, std::format(
        "[Il2CppOffline] Collect: registration search methods={} types={} images={} ver={} metaBase=0x{:X}",
        searchInput.methodCount,
        searchInput.typeDefCount,
        searchInput.imageCount,
        searchInput.version,
        metaBase));

    RegistrationInitResult registration{};
    RegistrationSearch search(pe, searchInput);

    uintptr_t er2CodeRegistration = 0;
    uintptr_t er2MetadataRegistration = 0;
    bool registrationReady = false;
    if (TryFindRegistrationsWithEr2(moduleBase, moduleSize, metaBase, er2CodeRegistration, er2MetadataRegistration))
    {
        registrationReady = search.InitFromAddresses(
            er2CodeRegistration,
            er2MetadataRegistration,
            registration,
            error);
        if (!registrationReady)
        {
            DumpSdkLog(DumpSdkLogLevel::Warn,
                "[Il2CppOffline] er2 registration addresses rejected: " + error);
            error.clear();
        }
    }

    if (!registrationReady)
    {
        DumpSdkLog(DumpSdkLogLevel::Info,
            "[Il2CppOffline] Collect: fallback Sidecar-style RegistrationSearch");
        if (!search.FindAndInit(registration, error))
        {
            return false;
        }
    }

    if (registrationOut != nullptr)
    {
        *registrationOut = registration;
    }

    TypeNameResolver typeNames(metadata, pe, registration);

    for (size_t imageIndex = 0; imageIndex < metadata.ImageDefs().size(); ++imageIndex)
    {
        const auto& imageDef = metadata.ImageDefs()[imageIndex];
        CollectedAssembly assembly{};
        assembly.imageIndex = imageIndex;
        assembly.name = metadata.GetStringFromIndex(imageDef.nameIndex);
        assembly.fileName = assembly.name;
        assembly.typeStartIndex = static_cast<size_t>(imageDef.typeStart);

        for (uint32_t localType = 0; localType < imageDef.typeCount; ++localType)
        {
            const size_t typeIndex = static_cast<size_t>(imageDef.typeStart) + localType;
            if (typeIndex >= metadata.TypeDefs().size())
            {
                continue;
            }
            const auto& typeDef = metadata.TypeDefs()[typeIndex];

            CollectedType collectedType{};
            collectedType.index = localType;
            collectedType.name = metadata.GetStringFromIndex(typeDef.nameIndex);
            collectedType.namespaceName = metadata.GetStringFromIndex(typeDef.namespaceIndex);
            collectedType.assemblyName = assembly.name;
            collectedType.token = typeDef.token;
            collectedType.flags = typeDef.flags;
            collectedType.kind = ResolveTypeKind(typeDef);
            collectedType.isNested = typeDef.declaringTypeIndex != -1;
            collectedType.isGeneric = typeDef.genericContainerIndex >= 0;
            collectedType.isAbstract = (typeDef.flags & 0x80u) != 0;
            collectedType.isSealed = (typeDef.flags & 0x100u) != 0;
            collectedType.isPublic = (typeDef.flags & 0x7u) == 2;
            collectedType.accessModifier = AccessFromFieldFlags(typeDef.flags);

            if (typeDef.parentIndex >= 0 &&
                typeDef.parentIndex < static_cast<int32_t>(registration.types.size()))
            {
                collectedType.parentName = typeNames.ResolveByTypeIndex(typeDef.parentIndex, true);
            }

            for (uint16_t fieldIndex = 0; fieldIndex < typeDef.field_count; ++fieldIndex)
            {
                const size_t flatFieldIndex = static_cast<size_t>(typeDef.fieldStart) + fieldIndex;
                if (flatFieldIndex >= metadata.FieldDefs().size())
                {
                    continue;
                }
                const auto& fieldDef = metadata.FieldDefs()[flatFieldIndex];
                CollectedField field{};
                field.name = metadata.GetStringFromIndex(fieldDef.nameIndex);
                field.typeName = typeNames.ResolveByTypeIndex(fieldDef.typeIndex, true);
                field.token = fieldDef.token;
                field.flags = 0;
                field.isStatic = false;
                field.isLiteral = false;
                field.isReadOnly = false;
                field.accessModifier = collectedType.accessModifier;
                field.offset = GetFieldOffset(
                    pe,
                    registration,
                    static_cast<int>(typeIndex),
                    fieldIndex,
                    flatFieldIndex,
                    typeDef.IsValueType(),
                    field.isStatic);
                collectedType.fields.push_back(std::move(field));
            }

            for (uint16_t methodIndex = 0; methodIndex < typeDef.method_count; ++methodIndex)
            {
                const size_t flatMethodIndex = static_cast<size_t>(typeDef.methodStart) + methodIndex;
                if (flatMethodIndex >= metadata.MethodDefs().size())
                {
                    continue;
                }
                const auto& methodDef = metadata.MethodDefs()[flatMethodIndex];
                CollectedMethod method{};
                method.name = metadata.GetStringFromIndex(methodDef.nameIndex);
                method.returnType = typeNames.ResolveByTypeIndex(methodDef.returnType, true);
                method.token = methodDef.token;
                method.flags = methodDef.flags;
                method.iflags = methodDef.iflags;
                method.isStatic = (methodDef.flags & kMethodStatic) != 0;
                method.isInstance = !method.isStatic;
                method.isAbstract = (methodDef.flags & kMethodAbstract) != 0;
                method.isVirtual = (methodDef.flags & kMethodVirtual) != 0;
                method.isSealed = (methodDef.flags & kMethodFinal) != 0;
                method.accessModifier = AccessFromMethodFlags(methodDef.flags);
                method.address = GetMethodPointer(registration, metadata.Version(), assembly.fileName, methodDef);

                for (uint16_t paramIndex = 0; paramIndex < methodDef.parameterCount; ++paramIndex)
                {
                    const size_t flatParamIndex = static_cast<size_t>(methodDef.parameterStart) + paramIndex;
                    if (flatParamIndex >= metadata.ParameterDefs().size())
                    {
                        continue;
                    }
                    const auto& paramDef = metadata.ParameterDefs()[flatParamIndex];
                    CollectedParam param{};
                    param.name = metadata.GetStringFromIndex(paramDef.nameIndex);
                    param.typeName = typeNames.ResolveByTypeIndex(paramDef.typeIndex, true);
                    param.isIn = true;
                    method.params.push_back(std::move(param));
                }
                collectedType.methods.push_back(std::move(method));
            }

            for (uint16_t propertyIndex = 0; propertyIndex < typeDef.property_count; ++propertyIndex)
            {
                const size_t flatPropertyIndex = static_cast<size_t>(typeDef.propertyStart) + propertyIndex;
                if (flatPropertyIndex >= metadata.PropertyDefs().size())
                {
                    continue;
                }
                const auto& propertyDef = metadata.PropertyDefs()[flatPropertyIndex];
                CollectedProperty property{};
                property.name = metadata.GetStringFromIndex(propertyDef.nameIndex);
                property.token = propertyDef.token;
                property.hasGetter = propertyDef.get >= 0;
                property.hasSetter = propertyDef.set >= 0;
                collectedType.properties.push_back(std::move(property));
            }

            assembly.types.push_back(std::move(collectedType));
        }

        out.assemblies.push_back(std::move(assembly));
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Collect complete: {} assemblies, codeReg=0x{:X}, metaReg=0x{:X}",
            out.assemblies.size(),
            registration.codeRegistrationVa,
            registration.metadataRegistrationVa));
    return true;
}

} // namespace er2
