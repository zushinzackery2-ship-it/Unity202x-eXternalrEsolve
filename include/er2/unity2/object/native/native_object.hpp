#pragma once

#include <cstdint>

#include "../../../core/types.hpp"
#include "../../../mem/memory_read.hpp"
#include "../../core/offsets.hpp"

namespace er2
{

struct UnityPlayerRange
{
    std::uintptr_t base = 0;
    std::uint32_t size = 0;
};

inline bool IsProbablyUnityObject(const IMemoryAccessor& mem, std::uintptr_t obj, const Offsets& off, const UnityPlayerRange& unityPlayer)
{
    if (!IsCanonicalUserPtr(obj))
    {
        return false;
    }

    
    std::uintptr_t vtable = 0;
    if (!ReadPtr(mem, obj, vtable))
    {
        return false;
    }

    
    const std::uintptr_t moduleEnd = unityPlayer.base + static_cast<std::uintptr_t>(unityPlayer.size);
    if (vtable < unityPlayer.base || vtable >= moduleEnd)
    {
        return false;
    }

    
    std::uintptr_t managed = 0;
    if (!ReadPtr(mem, obj + off.unity_object_managed_ptr, managed))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(managed))
    {
        return false;
    }

    return true;
}

/// <summary>

/// Native -> managed (+0x28) -> klass (+0x00)
/// </summary>
inline bool ReadUnityObjectKlass(const IMemoryAccessor& mem, std::uintptr_t obj, const Offsets& off, std::uintptr_t& outKlass)
{
    outKlass = 0;

    if (!IsCanonicalUserPtr(obj))
    {
        return false;
    }

    
    std::uintptr_t managed = 0;
    if (!ReadPtr(mem, obj + off.unity_object_managed_ptr, managed))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(managed))
    {
        return false;
    }

    
    std::uintptr_t klass = 0;
    if (!ReadPtr(mem, managed + off.managed_object_klass, klass))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(klass))
    {
        return false;
    }

    outKlass = klass;
    return true;
}

} // namespace er2
