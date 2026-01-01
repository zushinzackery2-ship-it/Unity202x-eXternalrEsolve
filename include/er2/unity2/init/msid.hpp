#pragma once

#include <optional>
#include <vector>
#include "context.hpp"
#include "../msid/ms_id_to_pointer.hpp"
#include "../msid/enumerate_objects.hpp"
#include "../msid/find_objects_of_type_all.hpp"

namespace er2
{

inline std::uintptr_t MsIdSetPtr()
{
    if (!IsInited() || !g_ctx.msIdToPointerSlotVa) return 0;
    MsIdToPointerSet set;
    if (!er2::ReadMsIdToPointerSet(Mem(), g_ctx.msIdToPointerSlotVa, set)) return 0;
    return set.set;
}

inline std::uint32_t MsIdCount()
{
    if (!IsInited() || !g_ctx.msIdToPointerSlotVa) return 0;
    MsIdToPointerSet set;
    if (!er2::ReadMsIdToPointerSet(Mem(), g_ctx.msIdToPointerSlotVa, set)) return 0;
    std::uintptr_t entriesBase = 0;
    std::uint32_t capacity = 0;
    std::uint32_t count = 0;
    if (!er2::ReadMsIdEntriesHeader(Mem(), set, entriesBase, capacity, count)) return 0;
    return count;
}

inline bool EnumerateMsIdToPointerObjects(const EnumerateOptions& opt, const std::function<void(const ObjectInfo&)>& cb)
{
    if (!IsInited() || !g_ctx.msIdToPointerSlotVa) return false;
    return er2::EnumerateMsIdToPointerObjects(g_ctx.runtime, Mem(), g_ctx.msIdToPointerSlotVa, g_ctx.off, g_ctx.unityPlayerRange, opt, cb);
}

// FindObjectsOfTypeAll - Simple API
// Usage: auto results = er2::FindObjectsOfTypeAll("PlayerController");
//        auto results = er2::FindObjectsOfTypeAll("Transform");
inline std::vector<FindObjectsOfTypeAllResult> FindObjectsOfTypeAll(const char* className)
{
    std::vector<FindObjectsOfTypeAllResult> out;
    if (!IsInited() || !g_ctx.msIdToPointerSlotVa || !className) return out;
    er2::FindObjectsOfTypeAll(g_ctx.runtime, Mem(), g_ctx.msIdToPointerSlotVa, g_ctx.off, g_ctx.unityPlayerRange, className, out, nullptr);
    return out;
}

// FindObjectsOfTypeAll with namespace
// Usage: auto results = er2::FindObjectsOfTypeAll("UnityEngine", "Transform");
inline std::vector<FindObjectsOfTypeAllResult> FindObjectsOfTypeAll(const char* nameSpace, const char* className)
{
    std::vector<FindObjectsOfTypeAllResult> out;
    if (!IsInited() || !g_ctx.msIdToPointerSlotVa || !className) return out;
    er2::FindObjectsOfTypeAll(g_ctx.runtime, Mem(), g_ctx.msIdToPointerSlotVa, g_ctx.off, g_ctx.unityPlayerRange, className, out, nameSpace);
    return out;
}

} // namespace er2
