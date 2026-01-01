#pragma once

#include <optional>

#include "context.hpp"

#include "../gom/gom_manager.hpp"
#include "../gom/gom_scan_validate.hpp"
#include "../gom/gom_walker.hpp"
#include "../gom/gom_list_node.hpp"
#include "../gom/gom_search_game_object.hpp"
#include "../object/native/native_game_object_name.hpp"

namespace er2
{

inline std::uintptr_t GomManager()
{
    if (!IsInited() || !g_ctx.gomGlobalSlotVa)
    {
        return 0;
    }

    std::uintptr_t manager = 0;
    if (!GetGomManagerFromGlobalSlot(Mem(), g_ctx.gomGlobalSlotVa, manager))
    {
        return 0;
    }
    return manager;
}

inline std::uintptr_t GomBucketsPtr()
{
    const std::uintptr_t manager = GomManager();
    if (!manager)
    {
        return 0;
    }

    std::uintptr_t buckets = 0;
    if (!GetGomBucketsPtr(Mem(), manager, g_ctx.gomOff, buckets))
    {
        return 0;
    }
    return buckets;
}

inline std::int32_t GomBucketCount()
{
    const std::uintptr_t manager = GomManager();
    if (!manager)
    {
        return 0;
    }

    std::int32_t count = 0;
    if (!GetGomBucketCount(Mem(), manager, g_ctx.gomOff, count))
    {
        return 0;
    }
    return count;
}

inline std::uintptr_t GomLocalGameObjectListHead()
{
    const std::uintptr_t manager = GomManager();
    if (!manager)
    {
        return 0;
    }

    std::uintptr_t listHead = 0;
    if (!GetGomLocalGameObjectListHead(Mem(), manager, g_ctx.gomOff, listHead))
    {
        return 0;
    }
    return listHead;
}

inline std::uintptr_t FindGameObjectThroughTag(std::int32_t tag)
{
    if (!IsInited() || !g_ctx.gomGlobalSlotVa)
    {
        return 0;
    }

    return er2::FindGameObjectThroughTag(Mem(), g_ctx.gomGlobalSlotVa, g_ctx.gomOff, tag);
}

inline ManagerCandidateCheck CheckGomManagerCandidate(std::uintptr_t manager)
{
    if (!IsInited())
    {
        return ManagerCandidateCheck{};
    }

    return er2::CheckGameObjectManagerCandidateBlindScan(Mem(), manager, g_ctx.gomOff);
}

inline bool EnumerateGameObjects(std::vector<GameObjectEntry>& out)
{
    if (!IsInited() || !g_ctx.gomGlobalSlotVa)
    {
        return false;
    }

    return er2::EnumerateGameObjects(Mem(), g_ctx.gomGlobalSlotVa, g_ctx.gomOff, out);
}

inline std::optional<std::vector<GameObjectEntry>> EnumerateGameObjects()
{
    std::vector<GameObjectEntry> out;
    if (!EnumerateGameObjects(out))
    {
        return std::nullopt;
    }
    return out;
}

inline bool GetListNodeNative(std::uintptr_t node, std::uintptr_t& outNative)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::GetListNodeNative(Mem(), node, g_ctx.gomOff, outNative);
}

inline std::uintptr_t GetListNodeNative(std::uintptr_t node)
{
    std::uintptr_t native = 0;
    if (!GetListNodeNative(node, native))
    {
        return 0;
    }
    return native;
}

inline bool GetListNodeNext(std::uintptr_t node, std::uintptr_t& outNext)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::GetListNodeNext(Mem(), node, g_ctx.gomOff, outNext);
}

inline std::uintptr_t GetListNodeNext(std::uintptr_t node)
{
    std::uintptr_t next = 0;
    if (!GetListNodeNext(node, next))
    {
        return 0;
    }
    return next;
}

inline std::uintptr_t GetGameObjectByName(const std::string& name)
{
    auto objs = EnumerateGameObjects();
    if (!objs) return 0;
    for (const auto& obj : *objs)
    {
        std::string goName;
        if (er2::ReadGameObjectName(Mem(), obj.nativeObject, g_ctx.off, goName) && goName == name)
            return obj.nativeObject;
    }
    return 0;
}

} // namespace er2
