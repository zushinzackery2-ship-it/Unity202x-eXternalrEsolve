#pragma once

#include <cstdint>

#include "../../mem/memory_read.hpp"
#include "gom_offsets.hpp"

namespace er2
{

inline bool GetComponentPool(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, std::uintptr_t& outPool)
{
    outPool = 0;
    if (!gameObject)
    {
        return false;
    }

    return ReadPtr(mem, gameObject + off.game_object.component_pool, outPool);
}

inline bool GetComponentCount(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, std::int32_t& outCount)
{
    outCount = 0;
    if (!gameObject)
    {
        return false;
    }

    return ReadValue(mem, gameObject + off.game_object.component_count, outCount);
}

inline bool GetComponentSlotTypeId(const IMemoryAccessor& mem, std::uintptr_t pool, const GomOffsets& off, int index, std::int32_t& outTypeId)
{
    outTypeId = 0;
    if (!pool || index < 0)
    {
        return false;
    }

    const std::uintptr_t slot = pool + static_cast<std::uintptr_t>(index) * static_cast<std::uintptr_t>(off.pool.slot_stride);
    return ReadValue(mem, slot + off.pool.slot_type_id, outTypeId);
}

inline bool GetComponentSlotNative(const IMemoryAccessor& mem, std::uintptr_t pool, const GomOffsets& off, int index, std::uintptr_t& outNative)
{
    outNative = 0;
    if (!pool || index < 0)
    {
        return false;
    }

    const std::uintptr_t slot = pool + static_cast<std::uintptr_t>(index) * static_cast<std::uintptr_t>(off.pool.slot_stride);
    return ReadPtr(mem, slot + off.pool.slot_native, outNative);
}

} // namespace er2
