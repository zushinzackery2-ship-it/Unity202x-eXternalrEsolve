#pragma once

#include <cstdint>
#include <cstddef>

#include "../Manager/Manager.hpp"
#include "../Bucket/Bucket.hpp"
#include "../ListNode/ListNode.hpp"

namespace UnityExternal
{

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
struct GOMScanTraceCounters
{
    std::uint32_t sizeOfImage = 0;
    std::uint32_t sectionsTotal = 0;
    std::uint32_t scanSections = 0;
    std::uint64_t chunkReads = 0;
    std::uint64_t chunkReadFails = 0;
    std::uint64_t slotTotal = 0;
    std::uint64_t likelyPtr = 0;

    std::uint64_t candTotal = 0;
    std::uint64_t failManagerNotPtr = 0;
    std::uint64_t failListHeadRead = 0;
    std::uint64_t failListValidate = 0;
    std::uint64_t failListEmpty = 0;
    std::uint64_t failBucketsPtr = 0;
    std::uint64_t failBucketCountRead = 0;
    std::uint64_t failBucketCountRange = 0;
    std::uint64_t bucketHashmaskKeyFail = 0;
    std::uint64_t bucketListHeadReadFail = 0;
    std::uint64_t bucketEmpty = 0;
    std::uint64_t failNoValidBucket = 0;
    std::uint64_t ok = 0;
    std::uint64_t validBucketTotal = 0;
    int bestScore = 0;
    std::uintptr_t bestGomGlobal = 0;
    std::uintptr_t bestManager = 0;
};

inline GOMScanTraceCounters& GetGOMScanTraceCounters()
{
    static GOMScanTraceCounters g;
    return g;
}

inline void ResetGOMScanTraceCounters()
{
    GetGOMScanTraceCounters() = GOMScanTraceCounters{};
}
#endif

inline bool IsLikelyPtr(std::uintptr_t p)
{
    if (!p) return false;
    if (p < 0x0000000000010000ull) return false;
    if (p > 0x00007FFFFFFFFFFFull) return false;
    if ((p & 7ull) != 0ull) return false;
    return true;
}

inline bool ValidateCircularDList(const IMemoryAccessor& mem,
                                 std::uintptr_t head,
                                 std::size_t& outSteps)
{
    outSteps = 0;
    if (!IsLikelyPtr(head)) return false;

    std::uintptr_t headPrev = 0;
    std::uintptr_t headNext = 0;
    if (!ReadPtr(mem, head + 0x0u, headPrev)) return false;
    if (!ReadPtr(mem, head + 0x8u, headNext)) return false;

    if (!IsLikelyPtr(headPrev) || !IsLikelyPtr(headNext)) return false;

    std::uintptr_t nextPrev = 0;
    if (!ReadPtr(mem, headNext + 0x0u, nextPrev)) return false;
    if (nextPrev != head) return false;

    std::uintptr_t prevNext = 0;
    if (!ReadPtr(mem, headPrev + 0x8u, prevNext)) return false;
    if (prevNext != head) return false;

    auto Next = [&](std::uintptr_t node, std::uintptr_t& outNext) -> bool
    {
        outNext = 0;
        if (!IsLikelyPtr(node)) return false;
        if (!ReadPtr(mem, node + 0x8u, outNext)) return false;
        if (!IsLikelyPtr(outNext)) return false;
        return true;
    };

    std::uintptr_t slow = head;
    std::uintptr_t fast = head;
    while (true)
    {
        if (!Next(slow, slow)) return false;

        if (!Next(fast, fast)) return false;
        if (!Next(fast, fast)) return false;

        if (slow == fast)
        {
            break;
        }
    }

    std::uintptr_t meet = slow;
    std::size_t cycleLen = 1;
    std::uintptr_t cycleCur = 0;
    if (!Next(meet, cycleCur)) return false;
    while (cycleCur != meet)
    {
        ++cycleLen;
        if (!Next(cycleCur, cycleCur)) return false;
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
        if (!Next(cycleCur, cycleCur)) return false;
    }
    if (!headInCycle) return false;

    std::uintptr_t cur = headNext;
    for (;;)
    {
        if (!IsLikelyPtr(cur)) return false;
        if (cur == head)
        {
            return true;
        }

        std::uintptr_t curPrev = 0;
        std::uintptr_t curNext = 0;
        if (!ReadPtr(mem, cur + 0x0u, curPrev)) return false;
        if (!ReadPtr(mem, cur + 0x8u, curNext)) return false;

        if (!IsLikelyPtr(curPrev) || !IsLikelyPtr(curNext)) return false;

        std::uintptr_t curNextPrev = 0;
        if (!ReadPtr(mem, curNext + 0x0u, curNextPrev)) return false;
        if (curNextPrev != cur) return false;

        std::uintptr_t curPrevNext = 0;
        if (!ReadPtr(mem, curPrev + 0x8u, curPrevNext)) return false;
        if (curPrevNext != cur) return false;

        cur = curNext;
        ++outSteps;
    }
}

inline bool IsListNonEmpty(const IMemoryAccessor& mem, std::uintptr_t listHead)
{
    if (!IsLikelyPtr(listHead)) return false;

    std::uintptr_t first = 0;
    if (!GetListNodeFirst(mem, listHead, first)) return false;

    if (!IsLikelyPtr(first)) return false;
    if (first == listHead) return false;

    return true;
}

struct ManagerCandidateCheck
{
    bool ok = false;
    int score = 0;
    std::uintptr_t manager = 0;
};

inline ManagerCandidateCheck CheckGameObjectManagerCandidateBlindScan(const IMemoryAccessor& mem, std::uintptr_t manager)
{
    ManagerCandidateCheck res{};
    res.manager = manager;

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
    GOMScanTraceCounters& tc = GetGOMScanTraceCounters();
    ++tc.candTotal;
#endif

    if (!IsLikelyPtr(manager))
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failManagerNotPtr;
#endif
        return res;
    }

    std::uintptr_t allManagersList = 0;
    if (!GetGOMLocalGameObjectListHead(mem, manager, allManagersList) || !IsLikelyPtr(allManagersList))
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failListHeadRead;
#endif
        return res;
    }

    std::size_t listSteps = 0;
    if (!ValidateCircularDList(mem, allManagersList, listSteps))
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failListValidate;
#endif
        return res;
    }

    if (listSteps == 0)
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failListEmpty;
#endif
        return res;
    }

    std::uintptr_t buckets = 0;
    if (!GetGOMBucketsPtr(mem, manager, buckets) || !IsLikelyPtr(buckets))
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failBucketsPtr;
#endif
        return res;
    }

    std::int32_t bucketCount = 0;
    if (!GetGOMBucketCount(mem, manager, bucketCount))
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failBucketCountRead;
#endif
        return res;
    }

    if (bucketCount <= 0 || bucketCount > 0x100000)
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failBucketCountRange;
#endif
        return res;
    }

    const std::uintptr_t bucketStride = 0x18u;
    const int scanCount = 1000;
    int endIdx = bucketCount;
    if (endIdx > scanCount) endIdx = scanCount;

    int validBucketCount = 0;
    for (int idx = 0; idx < endIdx; ++idx)
    {
        std::uintptr_t bucketPtr = buckets + (std::uintptr_t)idx * bucketStride;

        if (!IsBucketHashmaskKeyConsistent(mem, bucketPtr))
        {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
            ++tc.bucketHashmaskKeyFail;
#endif
            continue;
        }

        std::uintptr_t listHead = 0;
        if (!GetBucketListHead(mem, bucketPtr, listHead))
        {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
            ++tc.bucketListHeadReadFail;
#endif
            continue;
        }

        if (!IsListNonEmpty(mem, listHead))
        {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
            ++tc.bucketEmpty;
#endif
            continue;
        }

        ++validBucketCount;
    }

    if (validBucketCount <= 0)
    {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
        ++tc.failNoValidBucket;
#endif
        return res;
    }

    int score = 0;
    score += 20;
    score += 80;
    if (listSteps > 0) score += (int)((listSteps > 64) ? 64 : listSteps);
    score += (validBucketCount > 64) ? 64 : validBucketCount;
    score += 20;

    res.ok = true;
    res.score = score;

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
    ++tc.ok;
    tc.validBucketTotal += (std::uint64_t)validBucketCount;
    if (score > tc.bestScore) tc.bestScore = score;
#endif

    return res;
}

}
