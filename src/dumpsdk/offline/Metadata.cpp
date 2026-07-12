#include <er2/unity2/dumpsdk/offline/Metadata.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>

#include <format>
#include <stdexcept>
#include <type_traits>

namespace er2
{

namespace
{

class MetadataFormatError : public std::runtime_error
{
public:
    explicit MetadataFormatError(const char* message)
        : std::runtime_error(message)
    {
    }
};

constexpr uint32_t kMetadataSanity = 0xFAB11BAFu;

} // namespace

Metadata::Metadata(const uint8_t* data, size_t size)
{
    Load(data, size);
}

Metadata Metadata::CreateBoundView() const
{
    Metadata view;
    view.Bind(Data(), Size());
    view.version_ = version_;
    view.SetPosition(Position());
    return view;
}

void Metadata::Load(const uint8_t* data, size_t size)
{
    Bind(data, size);
    stringCache_.clear();
    imageDefs_.clear();
    typeDefs_.clear();
    methodDefs_.clear();
    fieldDefs_.clear();
    parameterDefs_.clear();
    propertyDefs_.clear();
    stringLiterals_.clear();
    Parse();
}

bool Metadata::VersionInRange(double version, double minVersion, double maxVersion, bool hasMin, bool hasMax)
{
    if (hasMin && version < minVersion)
    {
        return false;
    }
    if (hasMax && version > maxVersion)
    {
        return false;
    }
    return true;
}

void Metadata::Parse()
{
    if (Size() < 8)
    {
        throw MetadataFormatError("metadata buffer too small");
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Metadata::Parse begin size={}", Size()));

    SetPosition(0);
    const uint32_t sanity = ReadUInt32();
    const int32_t rawVersion = ReadInt32();
    if (sanity != kMetadataSanity)
    {
        throw MetadataFormatError("invalid metadata sanity");
    }
    if (rawVersion < 0 || rawVersion > 1000)
    {
        throw MetadataFormatError("invalid metadata file");
    }
    if (rawVersion < kMetadataVersionMin || rawVersion > kMetadataVersionMax)
    {
        throw MetadataFormatError("unsupported metadata version");
    }

    version_ = static_cast<double>(rawVersion);
    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Metadata::Parse version={}", rawVersion));

    SetPosition(0);
    header_ = ReadGlobalMetadataHeader();
    DetectVersion24Variants();
    ReadTables();

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Metadata::Parse end images={} types={} methods={}",
            imageDefs_.size(),
            typeDefs_.size(),
            methodDefs_.size()));
}

void Metadata::DetectVersion24Variants()
{
    if (static_cast<int>(version_) != 24)
    {
        return;
    }

    if (header_.stringLiteralOffset == 264u)
    {
        version_ = 24.2;
        SetPosition(0);
        header_ = ReadGlobalMetadataHeader();
    }
    else
    {
        const std::vector<Il2CppImageDefinition> probeImages = ReadMetadataTable<Il2CppImageDefinition>(
            header_.imagesOffset,
            header_.imagesSize,
            ImageDefinitionSize());
        for (const Il2CppImageDefinition& imageDef : probeImages)
        {
            if (imageDef.token != 1u)
            {
                version_ = 24.1;
                break;
            }
        }
    }

    imageDefs_ = ReadMetadataTable<Il2CppImageDefinition>(
        header_.imagesOffset,
        header_.imagesSize,
        ImageDefinitionSize());

    if (version_ == 24.2)
    {
        const int32_t assemblyEntrySize = 68;
        if (header_.assembliesSize / assemblyEntrySize < static_cast<int32_t>(imageDefs_.size()))
        {
            version_ = 24.4;
            SetPosition(0);
            header_ = ReadGlobalMetadataHeader();
            imageDefs_ = ReadMetadataTable<Il2CppImageDefinition>(
                header_.imagesOffset,
                header_.imagesSize,
                ImageDefinitionSize());
        }
    }

    if (version_ == 24.1)
    {
        const int32_t assemblyEntrySize = 64;
        if (header_.assembliesSize / assemblyEntrySize == static_cast<int32_t>(imageDefs_.size()))
        {
            const double savedVersion = version_;
            version_ = 24.4;
            ReadMetadataTable<uint8_t>(header_.assembliesOffset, header_.assembliesSize, 1);
            version_ = savedVersion;
        }
    }

    imageDefs_.clear();
}

void Metadata::ReadTables()
{
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables images");
    imageDefs_ = ReadMetadataTable<Il2CppImageDefinition>(
        header_.imagesOffset,
        header_.imagesSize,
        ImageDefinitionSize());
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables types");
    typeDefs_ = ReadMetadataTable<Il2CppTypeDefinition>(
        header_.typeDefinitionsOffset,
        header_.typeDefinitionsSize,
        TypeDefinitionSize());
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables methods");
    methodDefs_ = ReadMetadataTable<Il2CppMethodDefinition>(
        header_.methodsOffset,
        header_.methodsSize,
        MethodDefinitionSize());
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables params");
    parameterDefs_ = ReadMetadataTable<Il2CppParameterDefinition>(
        header_.parametersOffset,
        header_.parametersSize,
        ParameterDefinitionSize());
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables fields");
    fieldDefs_ = ReadMetadataTable<Il2CppFieldDefinition>(
        header_.fieldsOffset,
        header_.fieldsSize,
        FieldDefinitionSize());
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables properties");
    propertyDefs_ = ReadMetadataTable<Il2CppPropertyDefinition>(
        header_.propertiesOffset,
        header_.propertiesSize,
        PropertyDefinitionSize());
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables stringLiterals");
    stringLiterals_ = ReadMetadataTable<Il2CppStringLiteral>(
        header_.stringLiteralOffset,
        header_.stringLiteralSize,
        sizeof(Il2CppStringLiteral));
}

template<typename T>
std::vector<T> Metadata::ReadMetadataTable(uint32_t offset, int32_t byteSize, size_t elementSize) const
{
    std::vector<T> values;
    if (byteSize <= 0 || elementSize == 0)
    {
        return values;
    }
    if (byteSize % static_cast<int32_t>(elementSize) != 0)
    {
        throw MetadataFormatError("metadata table size is not aligned");
    }
    if (static_cast<size_t>(offset) > Size() ||
        static_cast<size_t>(byteSize) > Size() - static_cast<size_t>(offset))
    {
        throw MetadataFormatError("metadata table out of range");
    }

    const size_t count = static_cast<size_t>(byteSize) / elementSize;
    values.reserve(count);

    // Bound view only — NEVER Metadata(Data(), Size()) which re-enters Load/Parse.
    Metadata reader = CreateBoundView();
    reader.SetPosition(offset);
    for (size_t i = 0; i < count; ++i)
    {
        if constexpr (std::is_same_v<T, Il2CppImageDefinition>)
        {
            values.push_back(reader.ReadImageDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppTypeDefinition>)
        {
            values.push_back(reader.ReadTypeDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppMethodDefinition>)
        {
            values.push_back(reader.ReadMethodDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppParameterDefinition>)
        {
            values.push_back(reader.ReadParameterDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppFieldDefinition>)
        {
            values.push_back(reader.ReadFieldDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppPropertyDefinition>)
        {
            values.push_back(reader.ReadPropertyDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppStringLiteral>)
        {
            values.push_back(reader.ReadStringLiteral());
        }
        else
        {
            reader.Skip(elementSize);
            values.emplace_back();
        }
    }
    return values;
}

Il2CppGlobalMetadataHeader Metadata::ReadGlobalMetadataHeader() const
{
    Metadata reader = CreateBoundView();
    Il2CppGlobalMetadataHeader header{};

    header.sanity = reader.ReadUInt32();
    header.version = reader.ReadInt32();
    header.stringLiteralOffset = reader.ReadUInt32();
    header.stringLiteralSize = reader.ReadInt32();
    header.stringLiteralDataOffset = reader.ReadUInt32();
    header.stringLiteralDataSize = reader.ReadInt32();
    header.stringOffset = reader.ReadUInt32();
    header.stringSize = reader.ReadInt32();
    header.eventsOffset = reader.ReadUInt32();
    header.eventsSize = reader.ReadInt32();
    header.propertiesOffset = reader.ReadUInt32();
    header.propertiesSize = reader.ReadInt32();
    header.methodsOffset = reader.ReadUInt32();
    header.methodsSize = reader.ReadInt32();
    header.parameterDefaultValuesOffset = reader.ReadUInt32();
    header.parameterDefaultValuesSize = reader.ReadInt32();
    header.fieldDefaultValuesOffset = reader.ReadUInt32();
    header.fieldDefaultValuesSize = reader.ReadInt32();
    header.fieldAndParameterDefaultValueDataOffset = reader.ReadUInt32();
    header.fieldAndParameterDefaultValueDataSize = reader.ReadInt32();
    header.fieldMarshaledSizesOffset = reader.ReadInt32();
    header.fieldMarshaledSizesSize = reader.ReadInt32();
    header.parametersOffset = reader.ReadUInt32();
    header.parametersSize = reader.ReadInt32();
    header.fieldsOffset = reader.ReadUInt32();
    header.fieldsSize = reader.ReadInt32();
    header.genericParametersOffset = reader.ReadUInt32();
    header.genericParametersSize = reader.ReadInt32();
    header.genericParameterConstraintsOffset = reader.ReadUInt32();
    header.genericParameterConstraintsSize = reader.ReadInt32();
    header.genericContainersOffset = reader.ReadUInt32();
    header.genericContainersSize = reader.ReadInt32();
    header.nestedTypesOffset = reader.ReadUInt32();
    header.nestedTypesSize = reader.ReadInt32();
    header.interfacesOffset = reader.ReadUInt32();
    header.interfacesSize = reader.ReadInt32();
    header.vtableMethodsOffset = reader.ReadUInt32();
    header.vtableMethodsSize = reader.ReadInt32();
    header.interfaceOffsetsOffset = reader.ReadInt32();
    header.interfaceOffsetsSize = reader.ReadInt32();
    header.typeDefinitionsOffset = reader.ReadUInt32();
    header.typeDefinitionsSize = reader.ReadInt32();

    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        header.rgctxEntriesOffset = reader.ReadUInt32();
        header.rgctxEntriesCount = reader.ReadInt32();
    }

    header.imagesOffset = reader.ReadUInt32();
    header.imagesSize = reader.ReadInt32();
    header.assembliesOffset = reader.ReadUInt32();
    header.assembliesSize = reader.ReadInt32();

    if (VersionInRange(version_, 19.0, 24.5, true, true))
    {
        header.metadataUsageListsOffset = reader.ReadUInt32();
        header.metadataUsageListsCount = reader.ReadInt32();
        header.metadataUsagePairsOffset = reader.ReadUInt32();
        header.metadataUsagePairsCount = reader.ReadInt32();
    }

    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        header.fieldRefsOffset = reader.ReadUInt32();
        header.fieldRefsSize = reader.ReadInt32();
    }

    if (VersionInRange(version_, 20.0, 0.0, true, false))
    {
        header.referencedAssembliesOffset = reader.ReadInt32();
        header.referencedAssembliesSize = reader.ReadInt32();
    }

    if (VersionInRange(version_, 21.0, 27.2, true, true))
    {
        header.attributesInfoOffset = reader.ReadUInt32();
        header.attributesInfoCount = reader.ReadInt32();
        header.attributeTypesOffset = reader.ReadUInt32();
        header.attributeTypesCount = reader.ReadInt32();
    }

    if (VersionInRange(version_, 29.0, 0.0, true, false))
    {
        header.attributeDataOffset = reader.ReadUInt32();
        header.attributeDataSize = reader.ReadInt32();
        header.attributeDataRangeOffset = reader.ReadUInt32();
        header.attributeDataRangeSize = reader.ReadInt32();
    }

    if (VersionInRange(version_, 22.0, 0.0, true, false))
    {
        header.unresolvedVirtualCallParameterTypesOffset = reader.ReadInt32();
        header.unresolvedVirtualCallParameterTypesSize = reader.ReadInt32();
        header.unresolvedVirtualCallParameterRangesOffset = reader.ReadInt32();
        header.unresolvedVirtualCallParameterRangesSize = reader.ReadInt32();
    }

    if (VersionInRange(version_, 23.0, 0.0, true, false))
    {
        header.windowsRuntimeTypeNamesOffset = reader.ReadInt32();
        header.windowsRuntimeTypeNamesSize = reader.ReadInt32();
    }

    if (VersionInRange(version_, 27.0, 0.0, true, false))
    {
        header.windowsRuntimeStringsOffset = reader.ReadInt32();
        header.windowsRuntimeStringsSize = reader.ReadInt32();
    }

    if (VersionInRange(version_, 24.0, 0.0, true, false))
    {
        header.exportedTypeDefinitionsOffset = reader.ReadInt32();
        header.exportedTypeDefinitionsSize = reader.ReadInt32();
    }

    return header;
}

size_t Metadata::ImageDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) + sizeof(int32_t) + sizeof(uint32_t);
    if (VersionInRange(version_, 24.0, 0.0, true, false))
    {
        size += sizeof(int32_t) + sizeof(uint32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    if (VersionInRange(version_, 24.1, 0.0, true, false))
    {
        size += sizeof(int32_t) + sizeof(uint32_t);
    }
    return size;
}

Il2CppImageDefinition Metadata::ReadImageDefinition()
{
    Il2CppImageDefinition value{};
    value.nameIndex = ReadUInt32();
    value.assemblyIndex = ReadInt32();
    value.typeStart = ReadInt32();
    value.typeCount = ReadUInt32();
    if (VersionInRange(version_, 24.0, 0.0, true, false))
    {
        value.exportedTypeStart = ReadInt32();
        value.exportedTypeCount = ReadUInt32();
    }
    value.entryPointIndex = ReadInt32();
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    if (VersionInRange(version_, 24.1, 0.0, true, false))
    {
        value.customAttributeStart = ReadInt32();
        value.customAttributeCount = ReadUInt32();
    }
    return value;
}

size_t Metadata::TypeDefinitionSize() const
{
    size_t size = sizeof(uint32_t) * 2;
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.5, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t) * 3;
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        size += sizeof(int32_t) * 2;
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 22.0, false, true))
    {
        size += sizeof(int32_t) * 2;
    }
    if (VersionInRange(version_, 21.0, 22.0, true, true))
    {
        size += sizeof(int32_t) * 2;
    }
    size += sizeof(uint32_t);
    size += sizeof(int32_t) * 8;
    size += sizeof(uint16_t) * 8;
    size += sizeof(uint32_t);
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    return size;
}

Il2CppTypeDefinition Metadata::ReadTypeDefinition()
{
    Il2CppTypeDefinition value{};
    value.nameIndex = ReadUInt32();
    value.namespaceIndex = ReadUInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    value.byvalTypeIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.5, false, true))
    {
        value.byrefTypeIndex = ReadInt32();
    }
    value.declaringTypeIndex = ReadInt32();
    value.parentIndex = ReadInt32();
    value.elementTypeIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        value.rgctxStartIndex = ReadInt32();
        value.rgctxCount = ReadInt32();
    }
    value.genericContainerIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 22.0, false, true))
    {
        value.delegateWrapperFromManagedToNativeIndex = ReadInt32();
        value.marshalingFunctionsIndex = ReadInt32();
    }
    if (VersionInRange(version_, 21.0, 22.0, true, true))
    {
        value.ccwFunctionIndex = ReadInt32();
        value.guidIndex = ReadInt32();
    }
    value.flags = ReadUInt32();
    value.fieldStart = ReadInt32();
    value.methodStart = ReadInt32();
    value.eventStart = ReadInt32();
    value.propertyStart = ReadInt32();
    value.nestedTypesStart = ReadInt32();
    value.interfacesStart = ReadInt32();
    value.vtableStart = ReadInt32();
    value.interfaceOffsetsStart = ReadInt32();
    value.method_count = ReadUInt16();
    value.property_count = ReadUInt16();
    value.field_count = ReadUInt16();
    value.event_count = ReadUInt16();
    value.nested_type_count = ReadUInt16();
    value.vtable_count = ReadUInt16();
    value.interfaces_count = ReadUInt16();
    value.interface_offsets_count = ReadUInt16();
    value.bitfield = ReadUInt32();
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    return value;
}

size_t Metadata::MethodDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) * 2;
    if (VersionInRange(version_, 31.0, 0.0, true, false))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        size += sizeof(int32_t) * 5;
    }
    size += sizeof(uint32_t) + sizeof(uint16_t) * 4;
    return size;
}

Il2CppMethodDefinition Metadata::ReadMethodDefinition()
{
    Il2CppMethodDefinition value{};
    value.nameIndex = ReadUInt32();
    value.declaringType = ReadInt32();
    value.returnType = ReadInt32();
    if (VersionInRange(version_, 31.0, 0.0, true, false))
    {
        value.returnParameterToken = ReadInt32();
    }
    value.parameterStart = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    value.genericContainerIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        value.methodIndex = ReadInt32();
        value.invokerIndex = ReadInt32();
        value.delegateWrapperIndex = ReadInt32();
        value.rgctxStartIndex = ReadInt32();
        value.rgctxCount = ReadInt32();
    }
    value.token = ReadUInt32();
    value.flags = ReadUInt16();
    value.iflags = ReadUInt16();
    value.slot = ReadUInt16();
    value.parameterCount = ReadUInt16();
    return value;
}

size_t Metadata::ParameterDefinitionSize() const
{
    size_t size = sizeof(uint32_t) * 2;
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    return size;
}

Il2CppParameterDefinition Metadata::ReadParameterDefinition()
{
    Il2CppParameterDefinition value{};
    value.nameIndex = ReadUInt32();
    value.token = ReadUInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    value.typeIndex = ReadInt32();
    return value;
}

size_t Metadata::FieldDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    return size;
}

Il2CppFieldDefinition Metadata::ReadFieldDefinition()
{
    Il2CppFieldDefinition value{};
    value.nameIndex = ReadUInt32();
    value.typeIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    return value;
}

size_t Metadata::PropertyDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) * 2 + sizeof(uint32_t);
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    return size;
}

Il2CppPropertyDefinition Metadata::ReadPropertyDefinition()
{
    Il2CppPropertyDefinition value{};
    value.nameIndex = ReadUInt32();
    value.get = ReadInt32();
    value.set = ReadInt32();
    value.attrs = ReadUInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    return value;
}

Il2CppStringLiteral Metadata::ReadStringLiteral()
{
    Il2CppStringLiteral value{};
    value.length = ReadUInt32();
    value.dataIndex = ReadInt32();
    return value;
}

std::string Metadata::GetStringFromIndex(uint32_t index) const
{
    const auto found = stringCache_.find(index);
    if (found != stringCache_.end())
    {
        return found->second;
    }

    Metadata reader = CreateBoundView();
    const std::string value = reader.ReadStringToNull(header_.stringOffset + index);
    stringCache_.emplace(index, value);
    return value;
}

std::string Metadata::GetStringLiteralFromIndex(uint32_t index) const
{
    if (index >= stringLiterals_.size())
    {
        throw StreamBoundsError("string literal index out of range");
    }

    const Il2CppStringLiteral& literal = stringLiterals_[index];
    Metadata reader = CreateBoundView();
    reader.SetPosition(static_cast<size_t>(header_.stringLiteralDataOffset) + static_cast<size_t>(literal.dataIndex));
    const std::vector<uint8_t> bytes = reader.ReadBytes(literal.length);
    return std::string(bytes.begin(), bytes.end());
}

template std::vector<Il2CppImageDefinition> Metadata::ReadMetadataTable<Il2CppImageDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppTypeDefinition> Metadata::ReadMetadataTable<Il2CppTypeDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppMethodDefinition> Metadata::ReadMetadataTable<Il2CppMethodDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppParameterDefinition> Metadata::ReadMetadataTable<Il2CppParameterDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppFieldDefinition> Metadata::ReadMetadataTable<Il2CppFieldDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppPropertyDefinition> Metadata::ReadMetadataTable<Il2CppPropertyDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppStringLiteral> Metadata::ReadMetadataTable<Il2CppStringLiteral>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<uint8_t> Metadata::ReadMetadataTable<uint8_t>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;

} // namespace er2
