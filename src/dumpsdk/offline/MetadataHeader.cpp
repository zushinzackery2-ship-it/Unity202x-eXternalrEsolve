#include <er2/unity2/dumpsdk/offline/Metadata.h>

namespace er2
{

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

} // namespace er2
