#pragma once

#include <cstdint>

#include "../Globals/GOMGlobal.hpp"

namespace UnityExternal
{

inline bool GetComponentPool(const IMemoryAccessor& mem, std::uintptr_t gameObject, std::uintptr_t& outPool)
{
    outPool = 0;
    if (!gameObject) return false;
    return ReadPtr(mem, gameObject + 0x30, outPool) && outPool != 0;
}

inline std::uintptr_t GetComponentPool(std::uintptr_t gameObject)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetComponentPool(*acc, gameObject, out);
    return out;
}

inline bool GetComponentCount(const IMemoryAccessor& mem, std::uintptr_t gameObject, std::int32_t& outCount)
{
    outCount = 0;
    if (!gameObject) return false;
    return ReadInt32(mem, gameObject + 0x40, outCount);
}

inline std::int32_t GetComponentCount(std::uintptr_t gameObject)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::int32_t out = 0;
    GetComponentCount(*acc, gameObject, out);
    return out;
}

// Component pool slot helpers
inline bool GetComponentSlotTypeId(const IMemoryAccessor& mem, std::uintptr_t pool, int index, std::int32_t& outTypeId)
{
    outTypeId = 0;
    if (!pool || index < 0) return false;
    std::uintptr_t addr = pool + static_cast<std::uintptr_t>(index) * 16u;
    return ReadInt32(mem, addr, outTypeId);
}

inline std::int32_t GetComponentSlotTypeId(std::uintptr_t pool, int index)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::int32_t out = 0;
    GetComponentSlotTypeId(*acc, pool, index, out);
    return out;
}

inline bool GetComponentSlotNative(const IMemoryAccessor& mem, std::uintptr_t pool, int index, std::uintptr_t& outNative)
{
    outNative = 0;
    if (!pool || index < 0) return false;
    std::uintptr_t addr = pool + 0x8u + static_cast<std::uintptr_t>(index) * 0x10u;
    return ReadPtr(mem, addr, outNative) && outNative != 0;
}

inline std::uintptr_t GetComponentSlotNative(std::uintptr_t pool, int index)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetComponentSlotNative(*acc, pool, index, out);
    return out;
}

// Component enabled state (+0x38)
inline bool GetComponentEnabled(const IMemoryAccessor& mem, std::uintptr_t nativeComponent, bool& outEnabled)
{
    outEnabled = true;
    if (!nativeComponent) return false;
    std::uint8_t val = 0;
    if (!mem.Read(nativeComponent + 0x38u, &val, 1)) return false;
    outEnabled = (val != 0);
    return true;
}

inline bool GetComponentEnabled(std::uintptr_t nativeComponent)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc || !nativeComponent) return false;
    bool enabled = true;
    GetComponentEnabled(*acc, nativeComponent, enabled);
    return enabled;
}

} // namespace UnityExternal
