#pragma once

#include <cstdint>

#include "../Globals/GOMGlobal.hpp"

namespace UnityExternal
{

inline bool GetGOMManagerFromGlobal(const IMemoryAccessor& mem, std::uintptr_t gomGlobal, std::uintptr_t& outManager)
{
    outManager = 0;
    if (!gomGlobal) return false;
    return ReadPtr(mem, gomGlobal, outManager) && outManager != 0;
}

inline std::uintptr_t GetGOMManagerFromGlobal(std::uintptr_t gomGlobal)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetGOMManagerFromGlobal(*acc, gomGlobal, out);
    return out;
}

inline std::uintptr_t GetGOMManager()
{
    return GetGOMManagerFromGlobal(GetGOMGlobal());
}

inline bool GetGOMBucketsPtr(const IMemoryAccessor& mem, std::uintptr_t manager, std::uintptr_t& outBuckets)
{
    outBuckets = 0;
    if (!manager) return false;
    return ReadPtr(mem, manager + 0x0, outBuckets) && outBuckets != 0;
}

inline std::uintptr_t GetGOMBucketsPtr(std::uintptr_t manager)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetGOMBucketsPtr(*acc, manager, out);
    return out;
}

inline bool GetGOMBucketCount(const IMemoryAccessor& mem, std::uintptr_t manager, std::int32_t& outCount)
{
    outCount = 0;
    if (!manager) return false;
    return ReadInt32(mem, manager + 0x8, outCount);
}

inline std::int32_t GetGOMBucketCount(std::uintptr_t manager)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::int32_t out = 0;
    GetGOMBucketCount(*acc, manager, out);
    return out;
}

} // namespace UnityExternal
