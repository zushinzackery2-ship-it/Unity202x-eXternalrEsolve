#pragma once

#include <cstdint>
#include <string>

#include "../../mem/memory_read.hpp"
#include "gom_offsets.hpp"
#include "gom_pool.hpp"
#include "../object/native/native_component.hpp"
#include "../object/managed/managed_object.hpp"
#include "../object/managed/managed_backend.hpp"

namespace er2
{

inline std::uintptr_t GetComponentThroughTypeId(const IMemoryAccessor& mem, const GomOffsets& off, std::uintptr_t gameObjectNative, std::int32_t typeId)
{
    if (!gameObjectNative || typeId == 0)
    {
        return 0;
    }

    std::uintptr_t pool = 0;
    if (!GetComponentPool(mem, gameObjectNative, off, pool) || !pool)
    {
        return 0;
    }

    std::int32_t count = 0;
    if (!GetComponentCount(mem, gameObjectNative, off, count) || count <= 0)
    {
        return 0;
    }

    if (count > 1024)
    {
        count = 1024;
    }

    for (std::int32_t i = 0; i < count; ++i)
    {
        std::int32_t slotTypeId = 0;
        if (!GetComponentSlotTypeId(mem, pool, off, i, slotTypeId))
        {
            continue;
        }

        if (slotTypeId != typeId)
        {
            continue;
        }

        std::uintptr_t nativeComp = 0;
        if (!GetComponentSlotNative(mem, pool, off, i, nativeComp))
        {
            continue;
        }

        return nativeComp;
    }

    return 0;
}

inline std::uintptr_t GetComponentThroughTypeName(ManagedBackend runtime, const IMemoryAccessor& mem, const Offsets& off, const GomOffsets& gomOff, std::uintptr_t gameObjectNative, const std::string& typeName)
{
    if (!gameObjectNative || typeName.empty())
    {
        return 0;
    }

    std::uintptr_t pool = 0;
    if (!GetComponentPool(mem, gameObjectNative, gomOff, pool) || !pool)
    {
        return 0;
    }

    std::int32_t count = 0;
    if (!GetComponentCount(mem, gameObjectNative, gomOff, count) || count <= 0)
    {
        return 0;
    }

    if (count > 1024)
    {
        count = 1024;
    }

    for (std::int32_t i = 0; i < count; ++i)
    {
        std::uintptr_t nativeComp = 0;
        if (!GetComponentSlotNative(mem, pool, gomOff, i, nativeComp) || !nativeComp)
        {
            continue;
        }

        std::uintptr_t managedComp = 0;
        if (!GetNativeComponentManaged(mem, nativeComp, gomOff, managedComp) || !managedComp)
        {
            continue;
        }

        TypeInfo info;
        if (!ReadManagedObjectTypeInfo(mem, managedComp, off, info))
        {
            continue;
        }

        if (info.name == typeName)
        {
            return nativeComp;
        }
    }

    return 0;
}

} // namespace er2
