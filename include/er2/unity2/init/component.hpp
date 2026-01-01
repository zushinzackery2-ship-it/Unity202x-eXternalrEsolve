#pragma once

#include "context.hpp"
#include "../gom/gom_search_component.hpp"
#include "gom.hpp"

namespace er2
{

inline std::uintptr_t GetComponentThroughTypeId(std::uintptr_t gameObjectNative, std::int32_t typeId)
{
    if (!IsInited() || !gameObjectNative || typeId == 0) return 0;
    return er2::GetComponentThroughTypeId(Mem(), g_ctx.gomOff, gameObjectNative, typeId);
}

inline std::uintptr_t GetComponentThroughTypeName(std::uintptr_t gameObjectNative, const char* typeName)
{
    if (!IsInited() || !gameObjectNative || !typeName || !typeName[0]) return 0;
    return er2::GetComponentThroughTypeName(g_ctx.runtime, Mem(), g_ctx.off, g_ctx.gomOff, gameObjectNative, std::string(typeName));
}

inline std::uintptr_t GetTransformComponent(std::uintptr_t gameObjectNative)
{
    if (!IsInited() || !gameObjectNative) return 0;

    std::uintptr_t pool = 0;
    if (!er2::GetComponentPool(Mem(), gameObjectNative, g_ctx.gomOff, pool) || !pool) return 0;

    std::int32_t count = 0;
    if (!er2::GetComponentCount(Mem(), gameObjectNative, g_ctx.gomOff, count) || count <= 0) return 0;
    if (count > 128) count = 128;

    for (std::int32_t i = 0; i < count; ++i)
    {
        std::uintptr_t nativeComp = 0;
        if (!er2::GetComponentSlotNative(Mem(), pool, g_ctx.gomOff, i, nativeComp) || !nativeComp) continue;

        std::uintptr_t managedComp = 0;
        if (!er2::GetNativeComponentManaged(Mem(), nativeComp, g_ctx.gomOff, managedComp) || !managedComp) continue;

        TypeInfo ti;
        if (!er2::ReadManagedObjectTypeInfo(g_ctx.runtime, Mem(), managedComp, g_ctx.off, ti)) continue;

        if (ti.namespaze == "UnityEngine" && ti.name == "Transform")
        {
            return nativeComp;
        }
    }

    return 0;
}

inline std::uintptr_t GetCameraComponent(std::uintptr_t gameObjectNative)
{
    if (!IsInited() || !gameObjectNative) return 0;

    std::uintptr_t pool = 0;
    if (!er2::GetComponentPool(Mem(), gameObjectNative, g_ctx.gomOff, pool) || !pool) return 0;

    std::int32_t count = 0;
    if (!er2::GetComponentCount(Mem(), gameObjectNative, g_ctx.gomOff, count) || count <= 0) return 0;
    if (count > 128) count = 128;

    for (std::int32_t i = 0; i < count; ++i)
    {
        std::uintptr_t nativeComp = 0;
        if (!er2::GetComponentSlotNative(Mem(), pool, g_ctx.gomOff, i, nativeComp) || !nativeComp) continue;

        std::uintptr_t managedComp = 0;
        if (!er2::GetNativeComponentManaged(Mem(), nativeComp, g_ctx.gomOff, managedComp) || !managedComp) continue;

        TypeInfo ti;
        if (!er2::ReadManagedObjectTypeInfo(g_ctx.runtime, Mem(), managedComp, g_ctx.off, ti)) continue;

        if (ti.namespaze == "UnityEngine" && ti.name == "Camera")
        {
            return nativeComp;
        }
    }

    return 0;
}

inline std::uintptr_t GetComponentByTypeName(std::uintptr_t gameObjectNative, const char* typeName)
{
    return GetComponentThroughTypeName(gameObjectNative, typeName);
}

} // namespace er2
