#pragma once

#include <cstdint>

namespace er2
{

struct Il2CppGlobalMetadataHeader
{
    uint32_t sanity = 0;
    int32_t version = 0;
    uint32_t stringLiteralOffset = 0;
    int32_t stringLiteralSize = 0;
    uint32_t stringLiteralDataOffset = 0;
    int32_t stringLiteralDataSize = 0;
    uint32_t stringOffset = 0;
    int32_t stringSize = 0;
    uint32_t eventsOffset = 0;
    int32_t eventsSize = 0;
    uint32_t propertiesOffset = 0;
    int32_t propertiesSize = 0;
    uint32_t methodsOffset = 0;
    int32_t methodsSize = 0;
    uint32_t parameterDefaultValuesOffset = 0;
    int32_t parameterDefaultValuesSize = 0;
    uint32_t fieldDefaultValuesOffset = 0;
    int32_t fieldDefaultValuesSize = 0;
    uint32_t fieldAndParameterDefaultValueDataOffset = 0;
    int32_t fieldAndParameterDefaultValueDataSize = 0;
    int32_t fieldMarshaledSizesOffset = 0;
    int32_t fieldMarshaledSizesSize = 0;
    uint32_t parametersOffset = 0;
    int32_t parametersSize = 0;
    uint32_t fieldsOffset = 0;
    int32_t fieldsSize = 0;
    uint32_t genericParametersOffset = 0;
    int32_t genericParametersSize = 0;
    uint32_t genericParameterConstraintsOffset = 0;
    int32_t genericParameterConstraintsSize = 0;
    uint32_t genericContainersOffset = 0;
    int32_t genericContainersSize = 0;
    uint32_t nestedTypesOffset = 0;
    int32_t nestedTypesSize = 0;
    uint32_t interfacesOffset = 0;
    int32_t interfacesSize = 0;
    uint32_t vtableMethodsOffset = 0;
    int32_t vtableMethodsSize = 0;
    int32_t interfaceOffsetsOffset = 0;
    int32_t interfaceOffsetsSize = 0;
    uint32_t typeDefinitionsOffset = 0;
    int32_t typeDefinitionsSize = 0;
    uint32_t rgctxEntriesOffset = 0;
    int32_t rgctxEntriesCount = 0;
    uint32_t imagesOffset = 0;
    int32_t imagesSize = 0;
    uint32_t assembliesOffset = 0;
    int32_t assembliesSize = 0;
    uint32_t metadataUsageListsOffset = 0;
    int32_t metadataUsageListsCount = 0;
    uint32_t metadataUsagePairsOffset = 0;
    int32_t metadataUsagePairsCount = 0;
    uint32_t fieldRefsOffset = 0;
    int32_t fieldRefsSize = 0;
    int32_t referencedAssembliesOffset = 0;
    int32_t referencedAssembliesSize = 0;
    uint32_t attributesInfoOffset = 0;
    int32_t attributesInfoCount = 0;
    uint32_t attributeTypesOffset = 0;
    int32_t attributeTypesCount = 0;
    uint32_t attributeDataOffset = 0;
    int32_t attributeDataSize = 0;
    uint32_t attributeDataRangeOffset = 0;
    int32_t attributeDataRangeSize = 0;
    int32_t unresolvedVirtualCallParameterTypesOffset = 0;
    int32_t unresolvedVirtualCallParameterTypesSize = 0;
    int32_t unresolvedVirtualCallParameterRangesOffset = 0;
    int32_t unresolvedVirtualCallParameterRangesSize = 0;
    int32_t windowsRuntimeTypeNamesOffset = 0;
    int32_t windowsRuntimeTypeNamesSize = 0;
    int32_t windowsRuntimeStringsOffset = 0;
    int32_t windowsRuntimeStringsSize = 0;
    int32_t exportedTypeDefinitionsOffset = 0;
    int32_t exportedTypeDefinitionsSize = 0;
};

} // namespace er2
