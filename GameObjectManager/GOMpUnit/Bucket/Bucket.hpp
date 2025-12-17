#pragma once

#include <cstdint>
#include <string>

#include "HashCalc.hpp"
#include "../Globals/GOMGlobal.hpp"
#include "../Manager/Manager.hpp"

namespace UnityExternal
{

struct BucketAnalysis
{
    std::uint32_t hashMask = 0;
    std::string info;
    std::uint32_t flags = 0;
    std::uint64_t key = 0;
    std::uintptr_t value = 0;
};

// Bucket structure: stride=24, value/listHead at +0x10
inline bool GetBucketValue(const IMemoryAccessor& mem, std::uintptr_t bucketPtr, std::uintptr_t& outValue)
{
    outValue = 0;
    if (!bucketPtr) return false;
    return ReadPtr(mem, bucketPtr + 0x10, outValue) && outValue != 0;
}

inline std::uintptr_t GetBucketValue(std::uintptr_t bucketPtr)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetBucketValue(*acc, bucketPtr, out);
    return out;
}

inline bool GetBucketListHead(const IMemoryAccessor& mem, std::uintptr_t bucketPtr, std::uintptr_t& outListHead)
{
    return GetBucketValue(mem, bucketPtr, outListHead);
}

inline std::uintptr_t GetBucketListHead(std::uintptr_t bucketPtr)
{
    return GetBucketValue(bucketPtr);
}

inline bool GetBucketHashmask(const IMemoryAccessor& mem, std::uintptr_t bucket, std::uint32_t& outMask)
{
    outMask = 0;
    if (!bucket) return false;
    return ReadValue(mem, bucket, outMask);
}

inline bool GetBucketHashmask(std::uintptr_t bucket, std::uint32_t& outMask)
{
    outMask = 0;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return false;
    return GetBucketHashmask(*acc, bucket, outMask);
}

inline std::uint32_t GetBucketHashmask(std::uintptr_t bucket)
{
    std::uint32_t out = 0;
    GetBucketHashmask(bucket, out);
    return out;
}

inline bool GetBucketInfo(std::uintptr_t bucket, std::string& outPrefix)
{
    outPrefix.clear();
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc || !bucket) return false;

    constexpr std::size_t kLen = 12;
    char buf[kLen + 1] = {};
    if (!acc->Read(bucket + 4u, buf, kLen)) return false;
    buf[kLen] = '\0';

    std::size_t len = 0;
    for (std::size_t i = 0; i < kLen; ++i)
    {
        if (buf[i] == '\0') break;
        len = i + 1;
    }

    outPrefix.assign(buf, len);
    return true;
}

inline std::string GetBucketInfo(std::uintptr_t bucket)
{
    std::string out;
    GetBucketInfo(bucket, out);
    return out;
}

inline bool GetBucketFlags(std::uintptr_t bucket, std::uint32_t& outFlags)
{
    outFlags = 0;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc || !bucket) return false;

    std::uint32_t high = 0;
    if (!ReadValue(*acc, bucket + 0xCu, high)) return false;
    outFlags = high;
    return true;
}

inline std::uint32_t GetBucketFlags(std::uintptr_t bucket)
{
    std::uint32_t out = 0;
    GetBucketFlags(bucket, out);
    return out;
}

inline bool GetBucketFTagId(std::uintptr_t bucket, std::uint16_t& outTagId)
{
    outTagId = 0;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc || !bucket) return false;

    std::uint32_t low = 0;
    if (!ReadValue(*acc, bucket + 0x8u, low)) return false;
    outTagId = static_cast<std::uint16_t>(low & 0xFFFFu);
    return true;
}

inline std::uint16_t GetBucketFTagId(std::uintptr_t bucket)
{
    std::uint16_t out = 0;
    GetBucketFTagId(bucket, out);
    return out;
}

inline bool GetBucketKey(const IMemoryAccessor& mem, std::uintptr_t bucket, std::uint64_t& outKey)
{
    outKey = 0;
    if (!bucket) return false;
    return ReadValue(mem, bucket + 0x8u, outKey);
}

inline bool GetBucketKey(std::uintptr_t bucket, std::uint64_t& outKey)
{
    outKey = 0;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return false;
    return GetBucketKey(*acc, bucket, outKey);
}

inline std::uint64_t GetBucketKey(std::uintptr_t bucket)
{
    std::uint64_t out = 0;
    GetBucketKey(bucket, out);
    return out;
}

inline bool IsBucketHashmaskKeyConsistent(const IMemoryAccessor& mem, std::uintptr_t bucket)
{
    std::uint32_t mask = 0;
    if (!GetBucketHashmask(mem, bucket, mask)) return false;

    std::uint64_t key = 0;
    if (!GetBucketKey(mem, bucket, key)) return false;

    const std::uint32_t key32 = static_cast<std::uint32_t>(key);
    const std::uint32_t expected = CalHashmaskThrougTag(static_cast<std::int32_t>(key32));
    return mask == expected;
}

inline bool IsBucketHashmaskKeyConsistent(std::uintptr_t bucket)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return false;
    return IsBucketHashmaskKeyConsistent(*acc, bucket);
}

inline BucketAnalysis AnalyzeBucket(std::uintptr_t bucket)
{
    BucketAnalysis out;
    out.hashMask = GetBucketHashmask(bucket);
    out.info = GetBucketInfo(bucket);
    out.flags = GetBucketFlags(bucket);
    out.key = GetBucketKey(bucket);
    out.value = GetBucketValue(bucket);
    return out;
}

inline std::uintptr_t FindBucketThroughHashmask(std::uint32_t hashMask)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;

    std::uintptr_t gomGlobal = GetGOMGlobal();
    if (!gomGlobal) return 0;

    std::uintptr_t manager = 0;
    if (!GetGOMManagerFromGlobal(*acc, gomGlobal, manager)) return 0;

    std::uintptr_t buckets = 0;
    if (!GetGOMBucketsPtr(*acc, manager, buckets)) return 0;

    std::int32_t bucketCount = 0;
    if (!GetGOMBucketCount(*acc, manager, bucketCount) || bucketCount <= 0) return 0;

    const std::uintptr_t bucketStride = 24;

    for (int bi = 0; bi < bucketCount; ++bi)
    {
        std::uintptr_t bucketPtr = buckets + static_cast<std::uintptr_t>(bi) * bucketStride;

        std::uint32_t mask = 0;
        if (!GetBucketHashmask(*acc, bucketPtr, mask)) continue;
        if (mask != hashMask) continue;

        std::uint64_t key = 0;
        if (!GetBucketKey(*acc, bucketPtr, key)) continue;
        const std::uint32_t key32 = static_cast<std::uint32_t>(key);
        const std::uint32_t expected = CalHashmaskThrougTag(static_cast<std::int32_t>(key32));
        if (expected != mask) continue;

        return bucketPtr;
    }

    return 0;
}

inline std::uintptr_t FindBucketThroughTag(std::int32_t tagId)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;

    std::uintptr_t gomGlobal = GetGOMGlobal();
    if (!gomGlobal) return 0;

    std::uintptr_t manager = 0;
    if (!GetGOMManagerFromGlobal(*acc, gomGlobal, manager)) return 0;

    std::uintptr_t buckets = 0;
    if (!GetGOMBucketsPtr(*acc, manager, buckets)) return 0;

    std::int32_t bucketCount = 0;
    if (!GetGOMBucketCount(*acc, manager, bucketCount) || bucketCount <= 0) return 0;

    const std::uint32_t expectedMask = CalHashmaskThrougTag(tagId);
    const std::uint32_t expectedKey = static_cast<std::uint32_t>(tagId);
    const std::uintptr_t bucketStride = 24;

    for (int bi = 0; bi < bucketCount; ++bi)
    {
        std::uintptr_t bucketPtr = buckets + static_cast<std::uintptr_t>(bi) * bucketStride;

        std::uint32_t mask = 0;
        if (!GetBucketHashmask(*acc, bucketPtr, mask)) continue;
        if (mask != expectedMask) continue;

        std::uint64_t key = 0;
        if (!GetBucketKey(*acc, bucketPtr, key)) continue;
        if (static_cast<std::uint32_t>(key) != expectedKey) continue;

        return bucketPtr;
    }

    return 0;
}

} // namespace UnityExternal
