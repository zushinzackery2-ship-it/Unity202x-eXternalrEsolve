#pragma once

#include <cstdint>
#include <string>

#include "../../../mem/memory_read.hpp"
#include "../../gom/gom_offsets.hpp"
#include "../../core/offsets.hpp"

namespace er2
{

inline bool ReadGameObjectName(const IMemoryAccessor& mem, std::uintptr_t obj, const Offsets& off, std::string& outName)
{
    outName.clear();
    if (!obj)
    {
        return false;
    }

    std::uintptr_t namePtr = 0;
    if (!ReadPtr(mem, obj + off.game_object_name_ptr, namePtr) || !namePtr)
    {
        return false;
    }

    return ReadCString(mem, namePtr, outName);
}

} // namespace er2
