#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "../../mem/memory_read.hpp"
#include "../object/managed/il2cpp_class.hpp"
#include "ms_id_to_pointer.hpp"
#include "../core/offsets.hpp"
#include "../object/native/native_object.hpp"
#include "../object/native/native_game_object_name.hpp"
#include "../object/native/native_scriptable_object_name.hpp"

namespace er2
{

enum class ObjectKind : std::uint8_t
{
    GameObject = 1,
    ScriptableObject = 2,
};

struct ObjectInfo
{
    std::uintptr_t native = 0;
    std::uint32_t instanceId = 0;
    std::string objectName;
    std::string typeFullName;
    ObjectKind kind = ObjectKind::GameObject;
};

struct EnumerateOptions
{
    bool onlyGameObject = true;
    bool onlyScriptableObject = true;

    std::string filterLower;
};

inline bool MatchFilterLower(const std::string& s, const std::string& filterLower)
{
    if (filterLower.empty())
    {
        return true;
    }

    std::string tmp;
    tmp.reserve(s.size());
    for (char c : s)
    {
        if (c >= 'A' && c <= 'Z')
        {
            tmp.push_back(static_cast<char>(c - 'A' + 'a'));
        }
        else
        {
            tmp.push_back(c);
        }
    }

    return tmp.find(filterLower) != std::string::npos;
}

inline bool EnumerateMsIdToPointerObjects(
    const IMemoryAccessor& mem,
    std::uintptr_t msIdToPointerAddr,
    const Offsets& off,
    const UnityPlayerRange& unityPlayer,
    const EnumerateOptions& opt,
    const std::function<void(const ObjectInfo&)>& cb)
{
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

        const bool isGo = IsClassOrParent(mem, klass, off, "UnityEngine", "GameObject");
        const bool isSo = IsClassOrParent(mem, klass, off, "UnityEngine", "ScriptableObject");

        ObjectKind kind;
        if (isGo)
        {
            kind = ObjectKind::GameObject;
            if (!opt.onlyGameObject)
            {
                continue;
            }
        }
        else if (isSo)
        {
            kind = ObjectKind::ScriptableObject;
            if (!opt.onlyScriptableObject)
            {
                continue;
            }
        }
        else
        {
            continue;
        }

        std::string name;
        if (kind == ObjectKind::GameObject)
        {
            ReadGameObjectName(mem, obj, off, name);
        }
        else
        {
            ReadScriptableObjectName(mem, obj, off, name);
        }

        std::string ns;
        std::string cn;
        if (!ReadIl2CppClassName(mem, klass, off, ns, cn))
        {
            continue;
        }

        std::string full = ns.empty() ? cn : (ns + "." + cn);

        if (!MatchFilterLower(name, opt.filterLower) && !MatchFilterLower(full, opt.filterLower))
        {
            continue;
        }

        ObjectInfo info;
        info.native = obj;
        info.instanceId = key;
        info.objectName = name;
        info.typeFullName = full;
        info.kind = kind;

        cb(info);
    }

    return true;
}

} // namespace er2
