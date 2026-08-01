#include "SmokePeInternal.h"

#include <Windows.h>

#include <cstring>

namespace OfflineBehavior
{

bool BuildSmokeModule(const MetadataBlob& metadata, SmokeModule& out, std::string& error)
{
    out = {};

    const size_t metadataSize = (metadata.bytes.size() + 0xFFF) & ~static_cast<size_t>(0xFFF);
    uint8_t* metadataRegion = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, metadataSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (metadataRegion == nullptr)
    {
        error = "failed to allocate metadata region";
        return false;
    }
    std::memcpy(metadataRegion, metadata.bytes.data(), metadata.bytes.size());

    uint8_t* image = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, kImageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (image == nullptr)
    {
        VirtualFree(metadataRegion, 0, MEM_RELEASE);
        error = "failed to allocate module region";
        return false;
    }

    const uint64_t imageBase = reinterpret_cast<uint64_t>(image);
    const uint64_t metadataVa = reinterpret_cast<uint64_t>(metadataRegion);
    WritePeHeaders(image, imageBase);
    std::memset(image + kTextRva, 0xC3, kTextSize);

    DataSection data(image, imageBase);
    auto typeDefHandle = [&](int32_t index) -> uint64_t
    {
        return metadataVa + metadata.typeDefinitionsOffset +
            static_cast<uint64_t>(index) * kTypeDefinitionSize;
    };
    auto genericParameterHandle = [&](int32_t index) -> uint64_t
    {
        return metadataVa + metadata.genericParametersOffset +
            static_cast<uint64_t>(index) * kGenericParameterSize;
    };

    const uint32_t metadataRegistration = data.Reserve(16 * kPtr);
    const uint32_t codeRegistration = data.Reserve(15 * kPtr);
    const uint32_t codeGenModules = data.Reserve(kPtr);
    const uint32_t codeGenModule = data.Reserve(13 * kPtr);
    const uint32_t moduleName = data.Reserve(16);
    const uint32_t methodPointers = data.Reserve(kMethodCount * kPtr);
    const uint32_t invokerPointers = data.Reserve(kPtr);
    const uint32_t typesArray = data.Reserve(kRtCount * kPtr);
    const uint32_t typeRecords = data.Reserve(kRtCount * 16);
    const uint32_t fieldOffsetsArray = data.Reserve(kTypeCount * kPtr);
    const uint32_t fieldOffsetBlocks = data.Reserve(kTypeCount * kFieldOffsetStride);
    const uint32_t typeSizesArray = data.Reserve(kTypeCount * kPtr);
    const uint32_t typeSizesBlock = data.Reserve(kTypeCount * 4);
    const uint32_t genericMethodPointers = data.Reserve(kPtr);
    const uint32_t genericInstsArray = data.Reserve(kPtr);
    const uint32_t genericInstRecord = data.Reserve(2 * kPtr);
    const uint32_t genericInstArgv = data.Reserve(kPtr);
    const uint32_t genericClassRecord = data.Reserve(4 * kPtr);
    const uint32_t arrayTypeRecord = data.Reserve(4 * kPtr);
    const uint32_t methodSpecs = data.Reserve(12);
    const uint32_t genericMethodTable = data.Reserve(16);
    const uint32_t metadataPointer = data.Reserve(kPtr);

    if (data.Used() > kDataSize)
    {
        VirtualFree(metadataRegion, 0, MEM_RELEASE);
        VirtualFree(image, 0, MEM_RELEASE);
        error = "synthetic .data section overflowed";
        return false;
    }

    data.PutText(moduleName, kImageName);
    for (uint32_t i = 0; i < kMethodCount; ++i)
    {
        data.PutU64(methodPointers + i * kPtr, imageBase + kTextRva + i * kMethodStride);
    }
    data.PutU64(invokerPointers, imageBase + kTextRva);
    data.PutU64(genericMethodPointers, imageBase + kTextRva + kMethodCount * kMethodStride);

    data.PutU64(genericInstRecord, 1);
    data.PutU64(genericInstRecord + kPtr, data.Va(genericInstArgv));
    data.PutU64(genericInstArgv, data.Va(typeRecords + kRtInt32 * 16));
    data.PutU64(genericInstsArray, data.Va(genericInstRecord));

    data.PutU64(genericClassRecord, data.Va(typeRecords + kRtGeneric * 16));
    data.PutU64(genericClassRecord + kPtr, data.Va(genericInstRecord));
    data.PutU64(arrayTypeRecord, data.Va(typeRecords + kRtInt32 * 16));
    data.PutU32(arrayTypeRecord + kPtr, 2);

    const std::vector<RuntimeTypeSpec> specs = BuildRuntimeTypeSpecs();
    for (uint32_t i = 0; i < kRtCount; ++i)
    {
        const RuntimeTypeSpec& spec = specs[i];
        uint64_t datapoint = 0;
        if (i == kRtFieldMatrix)
        {
            datapoint = data.Va(arrayTypeRecord);
        }
        else if (i == kRtFieldGeneric)
        {
            datapoint = data.Va(genericClassRecord);
        }
        else if (spec.elementRuntimeType >= 0)
        {
            datapoint = data.Va(typeRecords + static_cast<uint32_t>(spec.elementRuntimeType) * 16);
        }
        else if (spec.genericParameterIndex >= 0)
        {
            datapoint = genericParameterHandle(spec.genericParameterIndex);
        }
        else if (spec.typeDefIndex >= 0)
        {
            datapoint = typeDefHandle(spec.typeDefIndex);
        }

        const uint32_t bits = (spec.attrs & 0xFFFFu) | (spec.typeEnum << 16) | (spec.byref << 29);
        data.PutU64(typeRecords + i * 16, datapoint);
        data.PutU32(typeRecords + i * 16 + 8, bits);
        data.PutU64(typesArray + i * kPtr, data.Va(typeRecords + i * 16));
    }

    for (uint32_t i = 0; i < kTypeCount; ++i)
    {
        data.PutU64(fieldOffsetsArray + i * kPtr, data.Va(fieldOffsetBlocks + i * kFieldOffsetStride));
        data.PutU64(typeSizesArray + i * kPtr, data.Va(typeSizesBlock));
    }
    data.PutU64(fieldOffsetsArray + kTypeNested * kPtr, 0);

    const uint32_t derivedOffsets = fieldOffsetBlocks + kTypeDerived * kFieldOffsetStride;
    data.PutU32(derivedOffsets + 0, 0x10);
    data.PutU32(derivedOffsets + 4, 0x18);
    data.PutU32(derivedOffsets + 8, 0x00);
    data.PutU32(derivedOffsets + 12, 0x20);
    data.PutU32(derivedOffsets + 16, 0x28);
    data.PutU32(derivedOffsets + 20, 0x30);

    data.PutU64(codeGenModule + 0 * kPtr, data.Va(moduleName));
    data.PutU64(codeGenModule + 1 * kPtr, kMethodCount);
    data.PutU64(codeGenModule + 2 * kPtr, data.Va(methodPointers));
    data.PutU64(codeGenModules, data.Va(codeGenModule));

    data.PutU64(codeRegistration + 2 * kPtr, 1);
    data.PutU64(codeRegistration + 3 * kPtr, data.Va(genericMethodPointers));
    data.PutU64(codeRegistration + 5 * kPtr, 1);
    data.PutU64(codeRegistration + 6 * kPtr, data.Va(invokerPointers));
    data.PutU64(codeRegistration + 13 * kPtr, 1);
    data.PutU64(codeRegistration + 14 * kPtr, data.Va(codeGenModules));

    data.PutI32(methodSpecs + 0, kMethodContainerGet);
    data.PutI32(methodSpecs + 4, 0);
    data.PutI32(methodSpecs + 8, -1);
    data.PutI32(genericMethodTable + 0, 0);
    data.PutI32(genericMethodTable + 4, 0);
    data.PutI32(genericMethodTable + 8, 0);
    data.PutI32(genericMethodTable + 12, 0);

    data.PutU64(metadataRegistration + 2 * kPtr, 1);
    data.PutU64(metadataRegistration + 3 * kPtr, data.Va(genericInstsArray));
    data.PutU64(metadataRegistration + 4 * kPtr, 1);
    data.PutU64(metadataRegistration + 5 * kPtr, data.Va(genericMethodTable));
    data.PutU64(metadataRegistration + 6 * kPtr, kRtCount);
    data.PutU64(metadataRegistration + 7 * kPtr, data.Va(typesArray));
    data.PutU64(metadataRegistration + 8 * kPtr, 1);
    data.PutU64(metadataRegistration + 9 * kPtr, data.Va(methodSpecs));
    data.PutU64(metadataRegistration + 10 * kPtr, kTypeCount);
    data.PutU64(metadataRegistration + 11 * kPtr, data.Va(fieldOffsetsArray));
    data.PutU64(metadataRegistration + 12 * kPtr, kTypeCount);
    data.PutU64(metadataRegistration + 13 * kPtr, data.Va(typeSizesArray));
    data.PutU64(metadataPointer, metadataVa);

    out.base = static_cast<uintptr_t>(imageBase);
    out.size = kImageSize;
    out.metadataAddress = static_cast<uintptr_t>(metadataVa);
    out.metadataSize = metadata.bytes.size();
    out.codeRegistrationVa = data.Va(codeRegistration);
    out.metadataRegistrationVa = data.Va(metadataRegistration);
    out.textRva = kTextRva;
    out.textFileOffset = kTextFileOffset;
    out.methodStride = kMethodStride;
    return true;
}

void ReleaseSmokeModule(SmokeModule& module)
{
    if (module.base != 0)
    {
        VirtualFree(reinterpret_cast<void*>(module.base), 0, MEM_RELEASE);
    }
    if (module.metadataAddress != 0)
    {
        VirtualFree(reinterpret_cast<void*>(module.metadataAddress), 0, MEM_RELEASE);
    }
    module = {};
}

} // namespace OfflineBehavior
