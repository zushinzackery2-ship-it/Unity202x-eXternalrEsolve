#include <er2/unity2/dumpsdk/offline/Metadata.h>

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

} // namespace

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
        else if constexpr (std::is_same_v<T, Il2CppEventDefinition>)
        {
            values.push_back(reader.ReadEventDefinition());
        }
        else if constexpr (std::is_same_v<T, Il2CppGenericContainer>)
        {
            values.push_back(reader.ReadGenericContainer());
        }
        else if constexpr (std::is_same_v<T, Il2CppGenericParameter>)
        {
            values.push_back(reader.ReadGenericParameter());
        }
        else if constexpr (std::is_same_v<T, Il2CppFieldRef>)
        {
            values.push_back(reader.ReadFieldRef());
        }
        else if constexpr (std::is_same_v<T, Il2CppFieldDefaultValue>)
        {
            values.push_back(reader.ReadFieldDefaultValue());
        }
        else if constexpr (std::is_same_v<T, Il2CppParameterDefaultValue>)
        {
            values.push_back(reader.ReadParameterDefaultValue());
        }
        else if constexpr (std::is_same_v<T, Il2CppCustomAttributeTypeRange>)
        {
            values.push_back(reader.ReadCustomAttributeTypeRange());
        }
        else if constexpr (std::is_same_v<T, Il2CppCustomAttributeDataRange>)
        {
            values.push_back(reader.ReadCustomAttributeDataRange());
        }
        else if constexpr (std::is_same_v<T, Il2CppMetadataUsageList>)
        {
            values.push_back(reader.ReadMetadataUsageList());
        }
        else if constexpr (std::is_same_v<T, Il2CppMetadataUsagePair>)
        {
            values.push_back(reader.ReadMetadataUsagePair());
        }
        else
        {
            reader.Skip(elementSize);
            values.emplace_back();
        }
    }
    return values;
}

template<typename T>
std::vector<T> Metadata::ReadPrimitiveTable(uint32_t offset, int32_t byteSize) const
{
    static_assert(sizeof(T) == 4, "primitive metadata tables are 32-bit");
    std::vector<T> values;
    if (byteSize <= 0)
    {
        return values;
    }
    if (static_cast<size_t>(offset) > Size() ||
        static_cast<size_t>(byteSize) > Size() - static_cast<size_t>(offset))
    {
        throw MetadataFormatError("metadata primitive table out of range");
    }

    const size_t count = static_cast<size_t>(byteSize) / sizeof(T);
    values.reserve(count);
    Metadata reader = CreateBoundView();
    reader.SetPosition(offset);
    for (size_t i = 0; i < count; ++i)
    {
        values.push_back(static_cast<T>(reader.ReadUInt32()));
    }
    return values;
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
template std::vector<Il2CppEventDefinition> Metadata::ReadMetadataTable<Il2CppEventDefinition>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppGenericContainer> Metadata::ReadMetadataTable<Il2CppGenericContainer>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppGenericParameter> Metadata::ReadMetadataTable<Il2CppGenericParameter>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppFieldRef> Metadata::ReadMetadataTable<Il2CppFieldRef>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppFieldDefaultValue> Metadata::ReadMetadataTable<Il2CppFieldDefaultValue>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppParameterDefaultValue> Metadata::ReadMetadataTable<Il2CppParameterDefaultValue>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppCustomAttributeTypeRange> Metadata::ReadMetadataTable<Il2CppCustomAttributeTypeRange>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppCustomAttributeDataRange> Metadata::ReadMetadataTable<Il2CppCustomAttributeDataRange>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppMetadataUsageList> Metadata::ReadMetadataTable<Il2CppMetadataUsageList>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<Il2CppMetadataUsagePair> Metadata::ReadMetadataTable<Il2CppMetadataUsagePair>(
    uint32_t offset, int32_t byteSize, size_t elementSize) const;
template std::vector<int32_t> Metadata::ReadPrimitiveTable<int32_t>(
    uint32_t offset, int32_t byteSize) const;

} // namespace er2
