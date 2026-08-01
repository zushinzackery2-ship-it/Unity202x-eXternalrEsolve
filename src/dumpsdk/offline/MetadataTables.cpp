#include <er2/unity2/dumpsdk/offline/Metadata.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>

namespace er2
{

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

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables events/interfaces/generics");
    eventDefs_ = ReadMetadataTable<Il2CppEventDefinition>(
        header_.eventsOffset,
        header_.eventsSize,
        EventDefinitionSize());
    interfaceIndices_ = ReadPrimitiveTable<int32_t>(
        header_.interfacesOffset,
        header_.interfacesSize);
    genericContainers_ = ReadMetadataTable<Il2CppGenericContainer>(
        header_.genericContainersOffset,
        header_.genericContainersSize,
        sizeof(int32_t) * 4);
    genericParameters_ = ReadMetadataTable<Il2CppGenericParameter>(
        header_.genericParametersOffset,
        header_.genericParametersSize,
        GenericParameterSize());

    if (version_ > 16.0)
    {
        fieldRefs_ = ReadMetadataTable<Il2CppFieldRef>(
            header_.fieldRefsOffset,
            header_.fieldRefsSize,
            sizeof(int32_t) * 2);
        if (version_ < 27.0)
        {
            metadataUsageLists_ = ReadMetadataTable<Il2CppMetadataUsageList>(
                header_.metadataUsageListsOffset,
                header_.metadataUsageListsCount,
                sizeof(uint32_t) * 2);
            metadataUsagePairs_ = ReadMetadataTable<Il2CppMetadataUsagePair>(
                header_.metadataUsagePairsOffset,
                header_.metadataUsagePairsCount,
                sizeof(uint32_t) * 2);
            ProcessMetadataUsage();
        }
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables default values");
    for (const Il2CppFieldDefaultValue& value : ReadMetadataTable<Il2CppFieldDefaultValue>(
             header_.fieldDefaultValuesOffset,
             header_.fieldDefaultValuesSize,
             sizeof(int32_t) * 3))
    {
        fieldDefaultValues_[value.fieldIndex] = value;
    }
    for (const Il2CppParameterDefaultValue& value : ReadMetadataTable<Il2CppParameterDefaultValue>(
             header_.parameterDefaultValuesOffset,
             header_.parameterDefaultValuesSize,
             sizeof(int32_t) * 3))
    {
        parameterDefaultValues_[value.parameterIndex] = value;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] Metadata::ReadTables attributes");
    if (version_ > 20.0 && version_ < 29.0)
    {
        attributeTypeRanges_ = ReadMetadataTable<Il2CppCustomAttributeTypeRange>(
            header_.attributesInfoOffset,
            header_.attributesInfoCount,
            CustomAttributeTypeRangeSize());
        attributeTypes_ = ReadPrimitiveTable<int32_t>(
            header_.attributeTypesOffset,
            header_.attributeTypesCount);
    }
    if (version_ >= 29.0)
    {
        attributeDataRanges_ = ReadMetadataTable<Il2CppCustomAttributeDataRange>(
            header_.attributeDataRangeOffset,
            header_.attributeDataRangeSize,
            sizeof(uint32_t) * 2);
    }
    if (version_ > 24.0)
    {
        BuildAttributeTokenMaps();
    }
}

void Metadata::BuildAttributeTokenMaps()
{
    attributeTokenMaps_.assign(imageDefs_.size(), {});
    for (size_t imageIndex = 0; imageIndex < imageDefs_.size(); ++imageIndex)
    {
        const Il2CppImageDefinition& imageDef = imageDefs_[imageIndex];
        std::unordered_map<uint32_t, int32_t>& map = attributeTokenMaps_[imageIndex];
        const int64_t end = static_cast<int64_t>(imageDef.customAttributeStart) +
            static_cast<int64_t>(imageDef.customAttributeCount);
        for (int64_t i = imageDef.customAttributeStart; i < end; ++i)
        {
            if (i < 0)
            {
                continue;
            }
            if (version_ >= 29.0)
            {
                if (static_cast<size_t>(i) >= attributeDataRanges_.size())
                {
                    continue;
                }
                map[attributeDataRanges_[static_cast<size_t>(i)].token] = static_cast<int32_t>(i);
            }
            else
            {
                if (static_cast<size_t>(i) >= attributeTypeRanges_.size())
                {
                    continue;
                }
                map[attributeTypeRanges_[static_cast<size_t>(i)].token] = static_cast<int32_t>(i);
            }
        }
    }
}

bool Metadata::TryGetFieldDefaultValue(int32_t fieldIndex, Il2CppFieldDefaultValue& value) const
{
    const auto found = fieldDefaultValues_.find(fieldIndex);
    if (found == fieldDefaultValues_.end())
    {
        return false;
    }
    value = found->second;
    return true;
}

bool Metadata::TryGetParameterDefaultValue(int32_t parameterIndex, Il2CppParameterDefaultValue& value) const
{
    const auto found = parameterDefaultValues_.find(parameterIndex);
    if (found == parameterDefaultValues_.end())
    {
        return false;
    }
    value = found->second;
    return true;
}

uint32_t Metadata::GetDefaultValueDataOffset(int32_t dataIndex) const
{
    return header_.fieldAndParameterDefaultValueDataOffset + static_cast<uint32_t>(dataIndex);
}

int32_t Metadata::GetCustomAttributeIndex(size_t imageIndex, int32_t customAttributeIndex, uint32_t token) const
{
    if (version_ <= 24.0)
    {
        return customAttributeIndex;
    }
    if (imageIndex >= attributeTokenMaps_.size())
    {
        return -1;
    }
    const auto& map = attributeTokenMaps_[imageIndex];
    const auto found = map.find(token);
    if (found == map.end())
    {
        return -1;
    }
    return found->second;
}

} // namespace er2
