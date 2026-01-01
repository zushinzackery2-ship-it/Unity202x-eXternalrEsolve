#pragma once

#include <cstddef>
#include <cstdint>

#include "../../core/types.hpp"
#include "../../mem/memory_read.hpp"

namespace er2
{

// MsIdToPointerSet 结构 (hash_set 头部)
// +0x00: entriesBase (指向 entry 数组, 8 bytes)
// +0x08: capacity (桶数量, 4 bytes)
// +0x0C: count (有效元素数量, 4 bytes)

// MsIdToPointerEntry 结构 (每个 entry 0x18 字节 = 3个8字节)
// 第1个8字节 (+0x00~+0x07): 低4字节=hashMask (0xFFFFFFFF=空, 0xFFFFFFFE=已删除)
// 第2个8字节 (+0x08~+0x0F): 低4字节=key (instance ID)
// 第3个8字节 (+0x10~+0x17): object指针

#pragma pack(push, 1)
struct MsIdToPointerEntryRaw
{
    std::uint32_t hashMask = 0;      // +0x00: hash mask (0xFFFFFFFF=空, 0xFFFFFFFE=已删除)
    std::uint32_t unk04 = 0;         // +0x04: 第1个8字节的高4字节
    std::uint32_t key = 0;           // +0x08: instance ID (第2个8字节的低4字节)
    std::uint32_t unk0C = 0;         // +0x0C: 第2个8字节的高4字节
    std::uintptr_t object = 0;       // +0x10: Object* 指针 (第3个8字节)
};
#pragma pack(pop)

static_assert(sizeof(MsIdToPointerEntryRaw) == 0x18, "MsIdToPointerEntryRaw size");

// 偏移常量
constexpr std::uint32_t kMsIdSetEntriesBase = 0x00;
constexpr std::uint32_t kMsIdSetCapacity = 0x08;
constexpr std::uint32_t kMsIdSetCount = 0x0C;
constexpr std::uint32_t kMsIdEntryHashMask = 0x00;
constexpr std::uint32_t kMsIdEntryKey = 0x08;      // 第2个8字节的低4字节
constexpr std::uint32_t kMsIdEntryObject = 0x10;
constexpr std::uint32_t kMsIdEntryStride = 0x18;

struct MsIdToPointerSet
{
    std::uintptr_t set = 0;
};

inline bool ReadMsIdToPointerSet(const IMemoryAccessor& mem, std::uintptr_t msIdToPointerAddr, MsIdToPointerSet& out)
{
    out = MsIdToPointerSet{};

    std::uintptr_t setPtr = 0;
    if (!ReadPtr(mem, msIdToPointerAddr, setPtr))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(setPtr))
    {
        return false;
    }

    out.set = setPtr;
    return true;
}

inline bool ReadMsIdEntriesHeader(const IMemoryAccessor& mem, const MsIdToPointerSet& set, std::uintptr_t& outEntriesBase, std::uint32_t& outCapacity, std::uint32_t& outCount)
{
    outEntriesBase = 0;
    outCapacity = 0;
    outCount = 0;

    if (!set.set)
    {
        return false;
    }

    std::uintptr_t entriesBase = 0;
    std::uint32_t capacity = 0;
    std::uint32_t count = 0;

    if (!ReadPtr(mem, set.set + kMsIdSetEntriesBase, entriesBase))
    {
        return false;
    }

    // capacity 在 +0x08
    if (!ReadValue(mem, set.set + kMsIdSetCapacity, capacity))
    {
        return false;
    }

    // count 在 +0x0C
    if (!ReadValue(mem, set.set + kMsIdSetCount, count))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(entriesBase))
    {
        return false;
    }

    // capacity 是桶数量，count 是有效元素数量
    if (capacity == 0 || capacity > 50000000)
    {
        return false;
    }

    if (count > capacity)
    {
        return false;
    }

    outEntriesBase = entriesBase;
    outCapacity = capacity;
    outCount = count;
    return true;
}

} // namespace er2
