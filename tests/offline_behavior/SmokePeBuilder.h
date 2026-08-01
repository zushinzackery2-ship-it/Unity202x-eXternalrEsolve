#pragma once

#include "SmokeMetadataBuilder.h"

#include <cstdint>
#include <string>

namespace OfflineBehavior
{

struct SmokeModule
{
    uintptr_t base = 0;
    uint32_t size = 0;
    uintptr_t metadataAddress = 0;
    size_t metadataSize = 0;
    uint64_t codeRegistrationVa = 0;
    uint64_t metadataRegistrationVa = 0;
    uint32_t textRva = 0;
    uint32_t textFileOffset = 0;
    uint32_t methodStride = 0;

    uint32_t MethodRva(uint32_t index) const
    {
        return textRva + index * methodStride;
    }

    uint32_t MethodFileOffset(uint32_t index) const
    {
        return textFileOffset + index * methodStride;
    }

    uint64_t MethodVa(uint32_t index) const
    {
        return static_cast<uint64_t>(base) + MethodRva(index);
    }
};

bool BuildSmokeModule(const MetadataBlob& metadata, SmokeModule& out, std::string& error);
void ReleaseSmokeModule(SmokeModule& module);

} // namespace OfflineBehavior
