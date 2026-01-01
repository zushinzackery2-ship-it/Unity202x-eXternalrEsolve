#pragma once

#include <cstdint>

#include "../../mem/memory_read.hpp"
#include "gom_offsets.hpp"

namespace er2
{

inline bool GetListNodeNext(const IMemoryAccessor& mem, std::uintptr_t node, const GomOffsets& off, std::uintptr_t& outNext)
{
    outNext = 0;
    if (!node)
    {
        return false;
    }

    return ReadPtr(mem, node + off.node.next, outNext);
}

inline bool GetListNodeNative(const IMemoryAccessor& mem, std::uintptr_t node, const GomOffsets& off, std::uintptr_t& outNative)
{
    outNative = 0;
    if (!node)
    {
        return false;
    }

    return ReadPtr(mem, node + off.node.native_object, outNative);
}

inline bool GetListNodeFirst(const IMemoryAccessor& mem, std::uintptr_t listHead, const GomOffsets& off, std::uintptr_t& outFirst)
{
    (void)mem;
    (void)off;
    outFirst = 0;
    if (!listHead)
    {
        return false;
    }

    
    outFirst = listHead;
    return true;
}

} // namespace er2
