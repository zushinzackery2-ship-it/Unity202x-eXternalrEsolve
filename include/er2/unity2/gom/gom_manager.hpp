#pragma once

#include <cstdint>

#include "../../mem/memory_read.hpp"
#include "gom_offsets.hpp"

namespace er2
{

inline bool GetGomManagerFromGlobalSlot(const IMemoryAccessor& mem, std::uintptr_t gomGlobalSlot, std::uintptr_t& outManager)
{
    outManager = 0;
    if (!gomGlobalSlot)
    {
        return false;
    }

    return ReadPtr(mem, gomGlobalSlot, outManager) && outManager != 0;
}

inline bool GetGomBucketsPtr(const IMemoryAccessor& mem, std::uintptr_t manager, const GomOffsets& off, std::uintptr_t& outBuckets)
{
    outBuckets = 0;
    if (!manager)
    {
        return false;
    }
    return ReadPtr(mem, manager + off.manager.buckets_ptr, outBuckets) && outBuckets != 0;
}

inline bool GetGomBucketCount(const IMemoryAccessor& mem, std::uintptr_t manager, const GomOffsets& off, std::int32_t& outCount)
{
    outCount = 0;
    if (!manager)
    {
        return false;
    }
    return ReadValue(mem, manager + off.manager.bucket_count, outCount);
}

inline bool GetGomLocalGameObjectListHead(const IMemoryAccessor& mem, std::uintptr_t manager, const GomOffsets& off, std::uintptr_t& outListHead)
{
    outListHead = 0;
    if (!manager)
    {
        return false;
    }

    return ReadPtr(mem, manager + off.manager.local_game_object_list_head, outListHead) && outListHead != 0;
}

} // namespace er2
