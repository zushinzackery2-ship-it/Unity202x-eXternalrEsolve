#pragma once

#include <cstdint>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "gom_hash_calc.hpp"
#include "gom_manager.hpp"
#include "gom_offsets.hpp"

namespace er2
{

/// <summary>


/// </summary>
inline bool GetBucketListHead(const IMemoryAccessor& mem, std::uintptr_t bucketPtr, const GomOffsets& off, std::uintptr_t& outListHead)
{
    outListHead = 0;
    if (!bucketPtr)
    {
        return false;
    }

    return ReadPtr(mem, bucketPtr + off.bucket.list_head, outListHead) && outListHead != 0;
}

inline bool GetBucketHashmask(const IMemoryAccessor& mem, std::uintptr_t bucketPtr, const GomOffsets& off, std::uint32_t& outMask)
{
    outMask = 0;
    if (!bucketPtr)
    {
        return false;
    }

    return ReadValue(mem, bucketPtr + off.bucket.hash_mask, outMask);
}

inline bool GetBucketKey(const IMemoryAccessor& mem, std::uintptr_t bucketPtr, const GomOffsets& off, std::uint64_t& outKey)
{
    outKey = 0;
    if (!bucketPtr)
    {
        return false;
    }

    return ReadValue(mem, bucketPtr + off.bucket.key, outKey);
}

inline bool IsBucketHashmaskKeyConsistent(const IMemoryAccessor& mem, std::uintptr_t bucketPtr, const GomOffsets& off)
{
    std::uint32_t mask = 0;
    if (!GetBucketHashmask(mem, bucketPtr, off, mask))
    {
        return false;
    }

    std::uint64_t key = 0;
    if (!GetBucketKey(mem, bucketPtr, off, key))
    {
        return false;
    }

    const std::uint32_t key32 = static_cast<std::uint32_t>(key);
    const std::uint32_t expected = CalHashmaskThrougTag(static_cast<std::int32_t>(key32));
    return mask == expected;
}

inline bool GetAllLinkedBucketsFromManager(const IMemoryAccessor& mem, std::uintptr_t managerAddress, const GomOffsets& off, std::vector<std::uintptr_t>& outBuckets)
{
    outBuckets.clear();

    if (!managerAddress)
    {
        return false;
    }

    std::uintptr_t bucketsBase = 0;
    if (!GetGomBucketsPtr(mem, managerAddress, off, bucketsBase))
    {
        return false;
    }

    std::int32_t bucketCount = 0;
    if (!GetGomBucketCount(mem, managerAddress, off, bucketCount) || bucketCount <= 0 || bucketCount > 0x100000)
    {
        return false;
    }

    for (std::int32_t bi = 0; bi < bucketCount; ++bi)
    {
        const std::uintptr_t bucketPtr = bucketsBase + static_cast<std::uintptr_t>(bi) * static_cast<std::uintptr_t>(off.bucket.stride);

        if (!IsBucketHashmaskKeyConsistent(mem, bucketPtr, off))
        {
            continue;
        }

        std::uintptr_t listHead = 0;
        if (!GetBucketListHead(mem, bucketPtr, off, listHead))
        {
            continue;
        }

        if (!listHead)
        {
            continue;
        }

        outBuckets.push_back(bucketPtr);
    }

    return !outBuckets.empty();
}

inline std::uintptr_t FindBucketThroughHashmask(const IMemoryAccessor& mem, std::uintptr_t manager, const GomOffsets& off, std::uint32_t hashMask)
{
    std::uintptr_t bucketsBase = 0;
    if (!GetGomBucketsPtr(mem, manager, off, bucketsBase))
    {
        return 0;
    }

    std::int32_t bucketCount = 0;
    if (!GetGomBucketCount(mem, manager, off, bucketCount) || bucketCount <= 0 || bucketCount > 0x100000)
    {
        return 0;
    }

    for (std::int32_t bi = 0; bi < bucketCount; ++bi)
    {
        const std::uintptr_t bucketPtr = bucketsBase + static_cast<std::uintptr_t>(bi) * static_cast<std::uintptr_t>(off.bucket.stride);

        std::uint32_t mask = 0;
        if (!GetBucketHashmask(mem, bucketPtr, off, mask) || mask != hashMask)
        {
            continue;
        }

        std::uint64_t key = 0;
        if (!GetBucketKey(mem, bucketPtr, off, key))
        {
            continue;
        }

        const std::uint32_t key32 = static_cast<std::uint32_t>(key);
        const std::uint32_t expected = CalHashmaskThrougTag(static_cast<std::int32_t>(key32));
        if (expected != mask)
        {
            continue;
        }

        return bucketPtr;
    }

    return 0;
}

inline std::uintptr_t FindBucketThroughTag(const IMemoryAccessor& mem, std::uintptr_t manager, const GomOffsets& off, std::int32_t tagId)
{
    std::uintptr_t bucketsBase = 0;
    if (!GetGomBucketsPtr(mem, manager, off, bucketsBase))
    {
        return 0;
    }

    std::int32_t bucketCount = 0;
    if (!GetGomBucketCount(mem, manager, off, bucketCount) || bucketCount <= 0 || bucketCount > 0x100000)
    {
        return 0;
    }
    const std::uint32_t expectedKey = static_cast<std::uint32_t>(tagId);

    for (std::int32_t bi = 0; bi < bucketCount; ++bi)
    {
        const std::uintptr_t bucketPtr = bucketsBase + static_cast<std::uintptr_t>(bi) * static_cast<std::uintptr_t>(off.bucket.stride);

        std::uint64_t key = 0;
        if (!GetBucketKey(mem, bucketPtr, off, key) || static_cast<std::uint32_t>(key) != expectedKey)
        {
            continue;
        }

        return bucketPtr;
    }

    return 0;
}

} // namespace er2
