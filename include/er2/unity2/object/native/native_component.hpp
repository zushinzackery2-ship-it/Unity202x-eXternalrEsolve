#pragma once

#include <cstdint>

#include "../../../mem/memory_read.hpp"
#include "../../gom/gom_offsets.hpp"

namespace er2
{

inline bool GetNativeComponentManaged(const IMemoryAccessor& mem, std::uintptr_t nativeComponent, const GomOffsets& off, std::uintptr_t& outManaged)
{
    outManaged = 0;
    if (!nativeComponent)
    {
        return false;
    }

    return ReadPtr(mem, nativeComponent + off.component.managed, outManaged) && outManaged != 0;
}

inline bool GetNativeComponentGameObject(const IMemoryAccessor& mem, std::uintptr_t nativeComponent, const GomOffsets& off, std::uintptr_t& outGameObject)
{
    outGameObject = 0;
    if (!nativeComponent)
    {
        return false;
    }

    return ReadPtr(mem, nativeComponent + off.component.game_object, outGameObject) && outGameObject != 0;
}

inline bool IsComponentEnabled(const IMemoryAccessor& mem, std::uintptr_t nativeComponent, const GomOffsets& off)
{
    if (!nativeComponent)
    {
        return false;
    }

    std::uint8_t enabled = 0;
    if (!ReadValue(mem, nativeComponent + off.component.enabled, enabled))
    {
        return false;
    }

    return enabled != 0;
}

} // namespace er2
