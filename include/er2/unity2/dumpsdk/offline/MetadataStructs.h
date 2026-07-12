#pragma once

#include <cstdint>

namespace er2
{

constexpr int kMetadataVersionMin = 16;
constexpr int kMetadataVersionMax = 31;

enum class Il2CppMetadataUsage : uint32_t
{
    kIl2CppMetadataUsageInvalid = 0,
    kIl2CppMetadataUsageTypeInfo = 1,
    kIl2CppMetadataUsageIl2CppType = 2,
    kIl2CppMetadataUsageMethodDef = 3,
    kIl2CppMetadataUsageFieldInfo = 4,
    kIl2CppMetadataUsageStringLiteral = 5,
    kIl2CppMetadataUsageMethodRef = 6,
};

enum Il2CppTypeDefinitionBitfield : uint32_t
{
    kIl2CppTypeValueType = 0x1,
    kIl2CppTypeEnumType = 0x2,
};

enum Il2CppMethodFlags : uint16_t
{
    kMethodMemberAccessMask = 0x0007,
    kMethodStatic = 0x0010,
    kMethodAbstract = 0x0400,
    kMethodFinal = 0x0020,
    kMethodVirtual = 0x0040,
};

enum Il2CppFieldFlags : uint32_t
{
    kFieldAccessMask = 0x0007,
    kFieldStatic = 0x0010,
    kFieldInitOnly = 0x0020,
    kFieldLiteral = 0x0040,
};

enum PeSectionCharacteristics : uint32_t
{
    kImageScnMemExecute = 0x20000000u,
    kImageScnMemRead = 0x40000000u,
    kImageScnMemWrite = 0x80000000u,
    kImageScnCntCode = 0x00000020u,
    kImageScnCntInitializedData = 0x00000040u,
    kImageScnCntUninitializedData = 0x00000080u,
};

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

struct Il2CppImageDefinition
{
    uint32_t nameIndex = 0;
    int32_t assemblyIndex = 0;
    int32_t typeStart = 0;
    uint32_t typeCount = 0;
    int32_t exportedTypeStart = 0;
    uint32_t exportedTypeCount = 0;
    int32_t entryPointIndex = 0;
    uint32_t token = 0;
    int32_t customAttributeStart = 0;
    uint32_t customAttributeCount = 0;
};

struct Il2CppTypeDefinition
{
    uint32_t nameIndex = 0;
    uint32_t namespaceIndex = 0;
    int32_t customAttributeIndex = 0;
    int32_t byvalTypeIndex = 0;
    int32_t byrefTypeIndex = 0;
    int32_t declaringTypeIndex = 0;
    int32_t parentIndex = 0;
    int32_t elementTypeIndex = 0;
    int32_t rgctxStartIndex = 0;
    int32_t rgctxCount = 0;
    int32_t genericContainerIndex = 0;
    int32_t delegateWrapperFromManagedToNativeIndex = 0;
    int32_t marshalingFunctionsIndex = 0;
    int32_t ccwFunctionIndex = 0;
    int32_t guidIndex = 0;
    uint32_t flags = 0;
    int32_t fieldStart = 0;
    int32_t methodStart = 0;
    int32_t eventStart = 0;
    int32_t propertyStart = 0;
    int32_t nestedTypesStart = 0;
    int32_t interfacesStart = 0;
    int32_t vtableStart = 0;
    int32_t interfaceOffsetsStart = 0;
    uint16_t method_count = 0;
    uint16_t property_count = 0;
    uint16_t field_count = 0;
    uint16_t event_count = 0;
    uint16_t nested_type_count = 0;
    uint16_t vtable_count = 0;
    uint16_t interfaces_count = 0;
    uint16_t interface_offsets_count = 0;
    uint32_t bitfield = 0;
    uint32_t token = 0;

    bool IsValueType() const
    {
        return (bitfield & kIl2CppTypeValueType) != 0;
    }

    bool IsEnum() const
    {
        return ((bitfield >> 1) & 0x1u) != 0;
    }
};

struct Il2CppMethodDefinition
{
    uint32_t nameIndex = 0;
    int32_t declaringType = 0;
    int32_t returnType = 0;
    int32_t returnParameterToken = 0;
    int32_t parameterStart = 0;
    int32_t customAttributeIndex = 0;
    int32_t genericContainerIndex = 0;
    int32_t methodIndex = 0;
    int32_t invokerIndex = 0;
    int32_t delegateWrapperIndex = 0;
    int32_t rgctxStartIndex = 0;
    int32_t rgctxCount = 0;
    uint32_t token = 0;
    uint16_t flags = 0;
    uint16_t iflags = 0;
    uint16_t slot = 0;
    uint16_t parameterCount = 0;
};

struct Il2CppParameterDefinition
{
    uint32_t nameIndex = 0;
    uint32_t token = 0;
    int32_t customAttributeIndex = 0;
    int32_t typeIndex = 0;
};

struct Il2CppFieldDefinition
{
    uint32_t nameIndex = 0;
    int32_t typeIndex = 0;
    int32_t customAttributeIndex = 0;
    uint32_t token = 0;
};

struct Il2CppPropertyDefinition
{
    uint32_t nameIndex = 0;
    int32_t get = 0;
    int32_t set = 0;
    uint32_t attrs = 0;
    int32_t customAttributeIndex = 0;
    uint32_t token = 0;
};

struct Il2CppStringLiteral
{
    uint32_t length = 0;
    int32_t dataIndex = 0;
};

struct DosHeader
{
    uint16_t magic = 0;
    uint16_t cblp = 0;
    uint16_t cp = 0;
    uint16_t crlc = 0;
    uint16_t cparhdr = 0;
    uint16_t minalloc = 0;
    uint16_t maxalloc = 0;
    uint16_t ss = 0;
    uint16_t sp = 0;
    uint16_t csum = 0;
    uint16_t ip = 0;
    uint16_t cs = 0;
    uint16_t lfarlc = 0;
    uint16_t ovno = 0;
    uint16_t res[4] = {};
    uint16_t oemid = 0;
    uint16_t oeminfo = 0;
    uint16_t res2[10] = {};
    uint32_t lfanew = 0;
};

struct PeFileHeader
{
    uint16_t machine = 0;
    uint16_t numberOfSections = 0;
    uint32_t timeDateStamp = 0;
    uint32_t pointerToSymbolTable = 0;
    uint32_t numberOfSymbols = 0;
    uint16_t sizeOfOptionalHeader = 0;
    uint16_t characteristics = 0;
};

struct PeOptionalHeader64
{
    uint16_t magic = 0;
    uint8_t majorLinkerVersion = 0;
    uint8_t minorLinkerVersion = 0;
    uint32_t sizeOfCode = 0;
    uint32_t sizeOfInitializedData = 0;
    uint32_t sizeOfUninitializedData = 0;
    uint32_t addressOfEntryPoint = 0;
    uint32_t baseOfCode = 0;
    uint64_t imageBase = 0;
    uint32_t sectionAlignment = 0;
    uint32_t fileAlignment = 0;
    uint16_t majorOperatingSystemVersion = 0;
    uint16_t minorOperatingSystemVersion = 0;
    uint16_t majorImageVersion = 0;
    uint16_t minorImageVersion = 0;
    uint16_t majorSubsystemVersion = 0;
    uint16_t minorSubsystemVersion = 0;
    uint32_t win32VersionValue = 0;
    uint32_t sizeOfImage = 0;
    uint32_t sizeOfHeaders = 0;
    uint32_t checkSum = 0;
    uint16_t subsystem = 0;
    uint16_t dllCharacteristics = 0;
    uint64_t sizeOfStackReserve = 0;
    uint64_t sizeOfStackCommit = 0;
    uint64_t sizeOfHeapReserve = 0;
    uint64_t sizeOfHeapCommit = 0;
    uint32_t loaderFlags = 0;
    uint32_t numberOfRvaAndSizes = 0;
};

struct PeSectionHeader
{
    char name[8] = {};
    uint32_t virtualSize = 0;
    uint32_t virtualAddress = 0;
    uint32_t sizeOfRawData = 0;
    uint32_t pointerToRawData = 0;
    uint32_t pointerToRelocations = 0;
    uint32_t pointerToLinenumbers = 0;
    uint16_t numberOfRelocations = 0;
    uint16_t numberOfLinenumbers = 0;
    uint32_t characteristics = 0;

    bool ContainsRva(uint32_t rva) const
    {
        const uint32_t span = virtualSize != 0 ? virtualSize : sizeOfRawData;
        return rva >= virtualAddress && rva < virtualAddress + span;
    }

    bool ContainsOffset(uint32_t offset) const
    {
        return offset >= pointerToRawData && offset < pointerToRawData + sizeOfRawData;
    }
};

} // namespace er2
