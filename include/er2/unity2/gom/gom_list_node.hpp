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
    outFirst = 0;
    if (!listHead)
    {
        return false;
    }

    // listHead是链表头结构，需要读next获取第一个节点
    return ReadPtr(mem, listHead + off.node.next, outFirst) && outFirst != 0;
}



} // namespace er2

