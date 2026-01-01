#pragma once

#include <optional>
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
    return er2::EnumerateMsIdToPointerObjects(Mem(), g_ctx.msIdToPointerSlotVa, g_ctx.off, g_ctx.unityPlayerRange, opt, cb);
}

inline bool FindObjectsOfTypeAll(const char* nameSpace, const char* className, std::vector<FindObjectsOfTypeAllResult>& out)
{
    if (!IsInited() || !g_ctx.msIdToPointerSlotVa) { out.clear(); return false; }
    return er2::FindObjectsOfTypeAll(Mem(), g_ctx.msIdToPointerSlotVa, g_ctx.off, g_ctx.unityPlayerRange, className, out, nameSpace);
}

} // namespace er2
