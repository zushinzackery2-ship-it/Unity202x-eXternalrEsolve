#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../../mem/memory_read.hpp"
#include "../../gom/gom_offsets.hpp"
#include "../../gom/gom_pool.hpp"

namespace er2
{

inline bool GetNativeGameObjectManaged(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, std::uintptr_t& outManaged)
{
    outManaged = 0;
    if (!gameObject)
    {
        return false;
    }

    return ReadPtr(mem, gameObject + off.game_object.managed, outManaged) && outManaged != 0;
}

inline bool GetNativeGameObjectTag(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, std::int32_t& outTag)
{
    outTag = 0;
    if (!gameObject)
    {
        return false;
    }

    std::int32_t raw = 0;
    if (!ReadValue(mem, gameObject + off.game_object.tag_raw, raw))
    {
        return false;
    }

    outTag = static_cast<std::int32_t>(static_cast<std::uint32_t>(raw) & 0xFFFFu);
    return true;
}

inline bool ReadNativeGameObjectName(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, std::string& outName)
{
    outName.clear();
    if (!gameObject)
    {
        return false;
    }

    std::uintptr_t namePtr = 0;
    if (!ReadPtr(mem, gameObject + off.game_object.name_ptr, namePtr) || !namePtr)
    {
        return false;
    }

    return ReadCString(mem, namePtr, outName);
}

inline bool GetNativeGameObjectComponentTypeIds(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, std::vector<std::int32_t>& outTypeIds)
{
    outTypeIds.clear();

    std::uintptr_t pool = 0;
    if (!GetComponentPool(mem, gameObject, off, pool) || !pool)
    {
        return false;
    }

    std::int32_t count = 0;
    if (!GetComponentCount(mem, gameObject, off, count) || count <= 0 || count > 1024)
    {
        return false;
    }

    outTypeIds.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        std::int32_t typeId = 0;
        if (!GetComponentSlotTypeId(mem, pool, off, i, typeId))
        {
            continue;
        }
        outTypeIds.push_back(typeId);
    }

    return true;
}

inline bool GetNativeGameObjectComponent(const IMemoryAccessor& mem, std::uintptr_t gameObject, const GomOffsets& off, int index, std::uintptr_t& outNativeComponent)
{
    outNativeComponent = 0;

    if (!gameObject || index < 0)
    {
        return false;
    }

    std::uintptr_t pool = 0;
    if (!GetComponentPool(mem, gameObject, off, pool) || !pool)
    {
        return false;
    }

    std::int32_t count = 0;
    if (!GetComponentCount(mem, gameObject, off, count) || index >= count)
    {
        return false;
    }

    return GetComponentSlotNative(mem, pool, off, index, outNativeComponent);
}

} // namespace er2
