#pragma once

#include <er2/unity2/dumpsdk/offline/BinaryStream.h>
#include <er2/unity2/dumpsdk/offline/MetadataStructs.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace er2
{

class Metadata : public BinaryStream
{
public:
    Metadata() = default;

    explicit Metadata(const uint8_t* data, size_t size);

    void Load(const uint8_t* data, size_t size);

    double Version() const
    {
        return version_;
    }

    const Il2CppGlobalMetadataHeader& Header() const
    {
        return header_;
    }

    const std::vector<Il2CppImageDefinition>& ImageDefs() const
    {
        return imageDefs_;
    }

    const std::vector<Il2CppTypeDefinition>& TypeDefs() const
    {
        return typeDefs_;
    }

    const std::vector<Il2CppMethodDefinition>& MethodDefs() const
    {
        return methodDefs_;
    }

    const std::vector<Il2CppFieldDefinition>& FieldDefs() const
    {
        return fieldDefs_;
    }

    const std::vector<Il2CppParameterDefinition>& ParameterDefs() const
    {
        return parameterDefs_;
    }

    const std::vector<Il2CppPropertyDefinition>& PropertyDefs() const
    {
        return propertyDefs_;
    }

    const std::vector<Il2CppStringLiteral>& StringLiterals() const
    {
        return stringLiterals_;
    }

    const std::vector<Il2CppEventDefinition>& EventDefs() const
    {
        return eventDefs_;
    }

    const std::vector<int32_t>& InterfaceIndices() const
    {
        return interfaceIndices_;
    }

    const std::vector<Il2CppGenericContainer>& GenericContainers() const
    {
        return genericContainers_;
    }

    const std::vector<Il2CppGenericParameter>& GenericParameters() const
    {
        return genericParameters_;
    }

    const std::vector<Il2CppFieldRef>& FieldRefs() const
    {
        return fieldRefs_;
    }

    const std::vector<Il2CppCustomAttributeTypeRange>& AttributeTypeRanges() const
    {
        return attributeTypeRanges_;
    }

    const std::vector<int32_t>& AttributeTypes() const
    {
        return attributeTypes_;
    }

    const std::vector<Il2CppCustomAttributeDataRange>& AttributeDataRanges() const
    {
        return attributeDataRanges_;
    }

    using MetadataUsageMap = std::map<uint32_t, uint32_t>;

    const std::unordered_map<uint32_t, MetadataUsageMap>& MetadataUsages() const
    {
        return metadataUsageDic_;
    }

    int64_t MetadataUsagesCount() const
    {
        return metadataUsagesCount_;
    }

    std::string GetStringFromIndex(uint32_t index) const;
    std::string GetStringLiteralFromIndex(uint32_t index) const;

    bool TryGetFieldDefaultValue(int32_t fieldIndex, Il2CppFieldDefaultValue& value) const;
    bool TryGetParameterDefaultValue(int32_t parameterIndex, Il2CppParameterDefaultValue& value) const;
    uint32_t GetDefaultValueDataOffset(int32_t dataIndex) const;

    // v25+ keys attributes by metadata token; older versions carry the index on
    // the definition record itself.
    int32_t GetCustomAttributeIndex(size_t imageIndex, int32_t customAttributeIndex, uint32_t token) const;

    static uint32_t GetEncodedIndexType(uint32_t index);
    uint32_t GetDecodedMethodIndex(uint32_t index) const;

    size_t TypeDefinitionRecordSize() const
    {
        return TypeDefinitionSize();
    }

    size_t GenericParameterRecordSize() const
    {
        return GenericParameterSize();
    }

private:
    Metadata CreateBoundView() const;

    void Parse();
    void DetectVersion24Variants();
    void ReadTables();
    void ProcessMetadataUsage();
    void BuildAttributeTokenMaps();

    template<typename T>
    std::vector<T> ReadMetadataTable(uint32_t offset, int32_t byteSize, size_t elementSize) const;
    template<typename T>
    std::vector<T> ReadPrimitiveTable(uint32_t offset, int32_t byteSize) const;

    Il2CppGlobalMetadataHeader ReadGlobalMetadataHeader() const;
    Il2CppImageDefinition ReadImageDefinition();
    Il2CppTypeDefinition ReadTypeDefinition();
    Il2CppMethodDefinition ReadMethodDefinition();
    Il2CppParameterDefinition ReadParameterDefinition();
    Il2CppFieldDefinition ReadFieldDefinition();
    Il2CppPropertyDefinition ReadPropertyDefinition();
    Il2CppStringLiteral ReadStringLiteral();
    Il2CppEventDefinition ReadEventDefinition();
    Il2CppGenericContainer ReadGenericContainer();
    Il2CppGenericParameter ReadGenericParameter();
    Il2CppFieldRef ReadFieldRef();
    Il2CppFieldDefaultValue ReadFieldDefaultValue();
    Il2CppParameterDefaultValue ReadParameterDefaultValue();
    Il2CppCustomAttributeTypeRange ReadCustomAttributeTypeRange();
    Il2CppCustomAttributeDataRange ReadCustomAttributeDataRange();
    Il2CppMetadataUsageList ReadMetadataUsageList();
    Il2CppMetadataUsagePair ReadMetadataUsagePair();

    size_t ImageDefinitionSize() const;
    size_t TypeDefinitionSize() const;
    size_t MethodDefinitionSize() const;
    size_t ParameterDefinitionSize() const;
    size_t FieldDefinitionSize() const;
    size_t PropertyDefinitionSize() const;
    size_t EventDefinitionSize() const;
    size_t GenericParameterSize() const;
    size_t CustomAttributeTypeRangeSize() const;

    static bool VersionInRange(double version, double minVersion, double maxVersion, bool hasMin, bool hasMax);

    double version_ = 0.0;
    Il2CppGlobalMetadataHeader header_{};
    std::vector<Il2CppImageDefinition> imageDefs_;
    std::vector<Il2CppTypeDefinition> typeDefs_;
    std::vector<Il2CppMethodDefinition> methodDefs_;
    std::vector<Il2CppFieldDefinition> fieldDefs_;
    std::vector<Il2CppParameterDefinition> parameterDefs_;
    std::vector<Il2CppPropertyDefinition> propertyDefs_;
    std::vector<Il2CppStringLiteral> stringLiterals_;
    std::vector<Il2CppEventDefinition> eventDefs_;
    std::vector<int32_t> interfaceIndices_;
    std::vector<Il2CppGenericContainer> genericContainers_;
    std::vector<Il2CppGenericParameter> genericParameters_;
    std::vector<Il2CppFieldRef> fieldRefs_;
    std::vector<Il2CppCustomAttributeTypeRange> attributeTypeRanges_;
    std::vector<int32_t> attributeTypes_;
    std::vector<Il2CppCustomAttributeDataRange> attributeDataRanges_;
    std::vector<Il2CppMetadataUsageList> metadataUsageLists_;
    std::vector<Il2CppMetadataUsagePair> metadataUsagePairs_;
    std::unordered_map<uint32_t, MetadataUsageMap> metadataUsageDic_;
    int64_t metadataUsagesCount_ = 0;
    std::unordered_map<int32_t, Il2CppFieldDefaultValue> fieldDefaultValues_;
    std::unordered_map<int32_t, Il2CppParameterDefaultValue> parameterDefaultValues_;
    // imageIndex -> (token -> attribute range index)
    std::vector<std::unordered_map<uint32_t, int32_t>> attributeTokenMaps_;
    mutable std::unordered_map<uint32_t, std::string> stringCache_;
};

} // namespace er2
