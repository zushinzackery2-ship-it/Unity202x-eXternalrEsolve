#pragma once

#include <cstdint>

#include "../../mem/memory_read.hpp"
#include "../metadata/registration_types.hpp"

namespace er2
{

inline bool DumpSdk6GetMetadataRegistrationTypes(
    const IMemoryAccessor& mem,
    std::uintptr_t metaReg,
    std::uint32_t il2cppVersion,
    std::uintptr_t& outTypesPtr,
    std::uint32_t& outTypesCount,
    std::uintptr_t& outFieldOffsetsPtr)
{
    outTypesPtr = 0;
    outTypesCount = 0;
    outFieldOffsetsPtr = 0;

    if (!metaReg || il2cppVersion == 0)
    {
        return false;
    }

    const detail_il2cpp_reg::MetaRegOffsets off = detail_il2cpp_reg::GetMetadataRegistrationOffsets(il2cppVersion);
    if (off.structSize <= 0 || off.typesCount < 0 || off.types < 0 || off.fieldOffsets < 0)
    {
        return false;
    }

    std::uint64_t typesCount64 = 0;
    std::uint64_t typesPtr64 = 0;
    std::uint64_t fieldOffsetsPtr64 = 0;

    if (!ReadValue(mem, metaReg + static_cast<std::uintptr_t>(off.typesCount), typesCount64))
    {
        return false;
    }
    if (!ReadValue(mem, metaReg + static_cast<std::uintptr_t>(off.types), typesPtr64))
    {
        return false;
    }
    if (!ReadValue(mem, metaReg + static_cast<std::uintptr_t>(off.fieldOffsets), fieldOffsetsPtr64))
    {
        return false;
    }

    if (typesCount64 == 0 || typesCount64 > 4000000ull)
    {
        return false;
    }

    outTypesCount = static_cast<std::uint32_t>(typesCount64);
    outTypesPtr = static_cast<std::uintptr_t>(typesPtr64);
    outFieldOffsetsPtr = static_cast<std::uintptr_t>(fieldOffsetsPtr64);
    return outTypesPtr != 0;
}

} // namespace er2
