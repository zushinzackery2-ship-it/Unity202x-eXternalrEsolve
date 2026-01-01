#pragma once

#include <cstdint>

#include "../../mem/memory_read.hpp"
#include "gom_bucket.hpp"
#include "gom_list_node.hpp"
#include "gom_manager.hpp"
#include "gom_offsets.hpp"

namespace er2
{

inline std::uintptr_t FindGameObjectThroughTag(const IMemoryAccessor& mem, std::uintptr_t gomGlobalSlot, const GomOffsets& off, std::int32_t tag)
{
    std::uintptr_t manager = 0;
    if (!GetGomManagerFromGlobalSlot(mem, gomGlobalSlot, manager))
    {
        return 0;
    }

    const std::uintptr_t bucket = FindBucketThroughTag(mem, manager, off, tag);
    if (!bucket)
    {
        return 0;
    }

    std::uintptr_t listHead = 0;
    if (!GetBucketListHead(mem, bucket, off, listHead))
    {
        return 0;
    }

    std::uintptr_t node = 0;
    if (!GetListNodeFirst(mem, listHead, off, node))
    {
        return 0;
    }

    std::uintptr_t nativeObject = 0;
    if (!GetListNodeNative(mem, node, off, nativeObject))
    {
        return 0;
    }

    return nativeObject;
}

} // namespace er2
