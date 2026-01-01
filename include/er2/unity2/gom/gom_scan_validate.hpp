#pragma once

#include <cstddef>
#include <cstdint>

#include "../../core/types.hpp"
#include "../../mem/memory_read.hpp"
#include "gom_bucket.hpp"
#include "gom_list_node.hpp"
#include "gom_manager.hpp"
#include "gom_offsets.hpp"

namespace er2
{

inline bool ValidateCircularDList(const IMemoryAccessor& mem, std::uintptr_t head, const GomOffsets& off, std::size_t& outSteps)
{
    outSteps = 0;
    if (!IsLikelyPtr(head))
    {
        return false;
    }

    std::uintptr_t headPrev = 0;
    std::uintptr_t headNext = 0;
    if (!ReadPtr(mem, head + off.node.prev, headPrev))
    {
        return false;
    }
    if (!ReadPtr(mem, head + off.node.next, headNext))
    {
        return false;
    }

    if (!IsLikelyPtr(headPrev) || !IsLikelyPtr(headNext))
    {
        return false;
    }

    std::uintptr_t nextPrev = 0;
    if (!ReadPtr(mem, headNext + off.node.prev, nextPrev))
    {
        return false;
    }
    if (nextPrev != head)
    {
        return false;
    }

    std::uintptr_t prevNext = 0;
    if (!ReadPtr(mem, headPrev + off.node.next, prevNext))
    {
        return false;
    }
    if (prevNext != head)
    {
        return false;
    }

    auto Next = [&](std::uintptr_t node, std::uintptr_t& outNext) -> bool
    {
        outNext = 0;
        if (!IsLikelyPtr(node))
        {
            return false;
        }
        if (!ReadPtr(mem, node + off.node.next, outNext))
        {
            return false;
        }
        if (!IsLikelyPtr(outNext))
        {
            return false;
        }
        return true;
    };

    std::uintptr_t slow = head;
    std::uintptr_t fast = head;
    while (true)
    {
        if (!Next(slow, slow))
        {
            return false;
        }

        if (!Next(fast, fast))
        {
            return false;
        }
        if (!Next(fast, fast))
        {
            return false;
        }

        if (slow == fast)
        {
            break;
        }
    }

    const std::uintptr_t meet = slow;

    std::size_t cycleLen = 1;
    std::uintptr_t cycleCur = 0;
    if (!Next(meet, cycleCur))
    {
        return false;
    }
    while (cycleCur != meet)
    {
        ++cycleLen;
        if (!Next(cycleCur, cycleCur))
        {
            return false;
        }
    }

    bool headInCycle = false;
    cycleCur = meet;
    for (std::size_t i = 0; i < cycleLen; ++i)
    {
        if (cycleCur == head)
        {
            headInCycle = true;
            break;
        }
        if (!Next(cycleCur, cycleCur))
        {
            return false;
        }
    }
    if (!headInCycle)
    {
        return false;
    }

    std::uintptr_t cur = headNext;
    for (;;)
    {
        if (!IsLikelyPtr(cur))
        {
            return false;
        }
        if (cur == head)
        {
            return true;
        }

        std::uintptr_t curPrev = 0;
        std::uintptr_t curNext = 0;
        if (!ReadPtr(mem, cur + off.node.prev, curPrev))
        {
            return false;
        }
        if (!ReadPtr(mem, cur + off.node.next, curNext))
        {
            return false;
        }

        if (!IsLikelyPtr(curPrev) || !IsLikelyPtr(curNext))
        {
            return false;
        }

        std::uintptr_t curNextPrev = 0;
        if (!ReadPtr(mem, curNext + off.node.prev, curNextPrev))
        {
            return false;
        }
        if (curNextPrev != cur)
        {
            return false;
        }

        std::uintptr_t curPrevNext = 0;
        if (!ReadPtr(mem, curPrev + off.node.next, curPrevNext))
        {
            return false;
        }
        if (curPrevNext != cur)
        {
            return false;
        }

        cur = curNext;
        ++outSteps;
    }
}

inline bool IsListNonEmpty(const IMemoryAccessor& mem, std::uintptr_t listHead, const GomOffsets& off)
{
    if (!IsLikelyPtr(listHead))
    {
        return false;
    }

    std::uintptr_t nativeObject = 0;
    if (!GetListNodeNative(mem, listHead, off, nativeObject))
    {
        return false;
    }

    return IsLikelyPtr(nativeObject);
}

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
