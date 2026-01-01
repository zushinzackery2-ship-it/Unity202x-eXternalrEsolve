#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "../object/managed/il2cpp_class.hpp"
#include "ms_id_to_pointer.hpp"
#include "../core/offsets.hpp"
#include "../object/native/native_object.hpp"

namespace er2
{

struct FindObjectsOfTypeAllResult
{
    std::uintptr_t native = 0;
    std::uint32_t instanceId = 0;
};

inline bool FindObjectsOfTypeAll(
    const IMemoryAccessor& mem,
    std::uintptr_t msIdToPointerAddr,
    const Offsets& off,
    const UnityPlayerRange& unityPlayer,
    const char* className,
    std::vector<FindObjectsOfTypeAllResult>& out,
    const char* nameSpace = nullptr)
{
    out.clear();

    if (!className || className[0] == '\0')
    {
        return false;
    }

    MsIdToPointerSet set;
    if (!ReadMsIdToPointerSet(mem, msIdToPointerAddr, set))
    {
        return false;
    }

    std::uintptr_t entriesBase = 0;
    std::uint32_t capacity = 0;
    std::uint32_t count = 0;
    if (!ReadMsIdEntriesHeader(mem, set, entriesBase, capacity, count))
    {
        return false;
    }

    // 遍历所有 capacity 个桶
    for (std::uint32_t i = 0; i < capacity; ++i)
    {
        const std::uintptr_t entry = entriesBase + static_cast<std::uintptr_t>(i) * kMsIdEntryStride;

        MsIdToPointerEntryRaw raw;
        if (!mem.Read(entry, &raw, sizeof(raw)))
        {
            continue;
        }

        // 检查 hashMask: 0xFFFFFFFF=空, 0xFFFFFFFE=已删除
        if (raw.hashMask >= 0xFFFFFFFEu)
        {
            continue;
        }

        const std::uint32_t key = raw.key;
        if (key == 0)
        {
            continue;
        }

        const std::uintptr_t obj = raw.object;
        if (!IsProbablyUnityObject(mem, obj, off, unityPlayer))
        {
            continue;
        }

        std::uintptr_t klass = 0;
        if (!ReadUnityObjectKlass(mem, obj, off, klass))
        {
            continue;
        }

        if (!IsClassOrParent(mem, klass, off, nameSpace, className))
        {
            continue;
        }

        FindObjectsOfTypeAllResult r;
        r.native = obj;
        r.instanceId = key;
        out.push_back(r);
    }

    return true;
}

} // namespace er2
