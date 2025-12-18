#pragma once

#include <cstdint>

#include "../Globals/GOMGlobal.hpp"

namespace UnityExternal
{

// Linked list node: next at +0x8, nativeObject at +0x10
inline bool GetListNodeFirst(const IMemoryAccessor& mem, std::uintptr_t listHead, std::uintptr_t& outNode)
{
    outNode = 0;
    if (!listHead) return false;
    if (!ReadPtr(mem, listHead + 0x8, outNode) || outNode == 0) return false;
    return outNode != listHead;
}

inline std::uintptr_t GetListNodeFirst(std::uintptr_t listHead)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetListNodeFirst(*acc, listHead, out);
    return out;
}

inline bool GetListNodeNext(const IMemoryAccessor& mem, std::uintptr_t node, std::uintptr_t& outNext)
{
    outNext = 0;
    if (!node) return false;
    return ReadPtr(mem, node + 0x8, outNext);
}

inline std::uintptr_t GetListNodeNext(std::uintptr_t node)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetListNodeNext(*acc, node, out);
    return out;
}

inline bool GetListNodeNative(const IMemoryAccessor& mem, std::uintptr_t node, std::uintptr_t& outNative)
{
    outNative = 0;
    if (!node) return false;
    return ReadPtr(mem, node + 0x10, outNative);
}

inline std::uintptr_t GetListNodeNative(std::uintptr_t node)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetListNodeNative(*acc, node, out);
    return out;
}

} // namespace UnityExternal
