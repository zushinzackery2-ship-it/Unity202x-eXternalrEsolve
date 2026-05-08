#pragma once

#include <cstddef>
#include <cstdint>

#include "../../core/types.hpp"
#include "../../mem/memory_read.hpp"
#include "gom_bucket.hpp"
#include "gom_manager.hpp"
#include "gom_offsets.hpp"
#include "validate_dlist.hpp"

namespace er2
{

struct ManagerCandidateCheck
{
    bool ok = false;
    int score = 0;
    std::uintptr_t manager = 0;
};

inline ManagerCandidateCheck CheckGameObjectManagerCandidateBlindScan(const IMemoryAccessor& mem, std::uintptr_t manager, const GomOffsets& off)
{
    ManagerCandidateCheck res;
    res.manager = manager;

    if (!IsLikelyPtr(manager))
    {
        return res;
    }

    std::uintptr_t allManagersList = 0;
    if (!GetGomLocalGameObjectListHead(mem, manager, off, allManagersList) || !IsLikelyPtr(allManagersList))
    {
        return res;
    }

    std::size_t listSteps = 0;
    if (!ValidateCircularDList(mem, allManagersList, off, listSteps))
    {
        return res;
    }

    if (listSteps == 0)
    {
        return res;
    }

    std::uintptr_t buckets = 0;
    if (!GetGomBucketsPtr(mem, manager, off, buckets) || !IsLikelyPtr(buckets))
    {
        return res;
    }

    std::int32_t bucketCount = 0;
    if (!GetGomBucketCount(mem, manager, off, bucketCount))
    {
        return res;
    }

    if (bucketCount <= 0 || bucketCount > 0x100000)
    {
        return res;
    }

    if (off.bucket.stride == 0)
    {
        return res;
    }

    const int scanCount = 1000;
    int endIdx = bucketCount;
    if (endIdx > scanCount)
    {
        endIdx = scanCount;
    }

    int validBucketCount = 0;
    for (int idx = 0; idx < endIdx; ++idx)
    {
        const std::uintptr_t bucketPtr = buckets + static_cast<std::uintptr_t>(idx) * static_cast<std::uintptr_t>(off.bucket.stride);

        if (!IsBucketHashmaskKeyConsistent(mem, bucketPtr, off))
        {
            continue;
        }

        std::uintptr_t listHead = 0;
        if (!GetBucketListHead(mem, bucketPtr, off, listHead))
        {
            continue;
        }

        if (!IsListNonEmpty(mem, listHead, off))
        {
            continue;
        }

        ++validBucketCount;
    }

    if (validBucketCount <= 0)
    {
        return res;
    }

    int score = 0;
    score += 20;
    score += 80;
    score += static_cast<int>((listSteps > 64) ? 64 : listSteps);
    score += (validBucketCount > 64) ? 64 : validBucketCount;
    score += 20;

    res.ok = true;
    res.score = score;
    return res;
}

} // namespace er2
