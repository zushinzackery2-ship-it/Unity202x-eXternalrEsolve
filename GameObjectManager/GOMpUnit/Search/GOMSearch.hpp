#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "../Walker/WalkerImpl.hpp"
#include "../../Native/NativeGameObject.hpp"

namespace UnityExternal
{

inline bool ContainsAllSubstrings(const std::string& text, std::initializer_list<std::string_view> parts)
{
    if (parts.size() == 0) return false;
    for (std::string_view p : parts)
    {
        if (p.empty()) continue;
        if (text.find(p.data(), 0, p.size()) == std::string::npos) return false;
    }
    return true;
}

inline std::vector<GameObjectEntry> EnumerateGameObjects()
{
    std::vector<GameObjectEntry> out;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return out;
    GOMWalker walker(*acc, GetDefaultRuntime());
    walker.EnumerateGameObjects(out);
    return out;
}

inline std::vector<std::uintptr_t> GetAllLinkedBuckets()
{
    std::vector<std::uintptr_t> out;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return out;
    std::uintptr_t gomGlobal = GetGOMGlobal();
    if (!gomGlobal) return out;
    GetAllLinkedBuckets(*acc, GetDefaultRuntime(), gomGlobal, out);
    return out;
}

// Fast tag search without materializing all GOs.
inline std::uintptr_t FindGameObjectThroughTag(std::int32_t tag)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;

    std::uintptr_t bucketPtr = FindBucketThroughTag(tag);
    if (!bucketPtr) return 0;

    std::uintptr_t listHead = 0;
    if (!GetBucketListHead(*acc, bucketPtr, listHead)) return 0;

    std::uintptr_t node = 0;
    if (!GetListNodeFirst(*acc, listHead, node)) return 0;

    std::unordered_set<std::uintptr_t> visited;

    for (; node;)
    {
        if (visited.find(node) != visited.end()) break;
        visited.insert(node);

        std::uintptr_t nativeObject = 0;
        if (!GetListNodeNative(*acc, node, nativeObject)) break;
        if (nativeObject)
        {
            NativeGameObject go(nativeObject);
            std::int32_t tagValue = 0;
            if (go.GetTag(tagValue) && tagValue == tag)
            {
                return nativeObject;
            }
        }

        std::uintptr_t next = 0;
        if (!GetListNodeNext(*acc, node, next) || !next || next == listHead) break;
        node = next;
    }

    return 0;
}

inline std::vector<ComponentEntry> EnumerateComponents()
{
    std::vector<ComponentEntry> out;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return out;
    GOMWalker walker(*acc, GetDefaultRuntime());
    walker.EnumerateComponents(out);
    return out;
}

inline std::uintptr_t FindGameObjectThroughName(const std::string& name)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    GOMWalker walker(*acc, GetDefaultRuntime());

    std::vector<GameObjectEntry> gameObjects;
    if (!walker.EnumerateGameObjects(gameObjects)) return 0;

    for (const auto& go : gameObjects)
    {
        if (!go.nativeObject) continue;
        NativeGameObject ngo(go.nativeObject);
        std::string goName = ngo.GetName();
        if (!goName.empty() && goName == name) return go.nativeObject;
    }
    return 0;
}

inline std::vector<std::uintptr_t> FindGameObjectsThroughNameContainsAll(std::initializer_list<std::string_view> parts)
{
    std::vector<std::uintptr_t> out;
    if (parts.size() == 0) return out;

    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return out;
    GOMWalker walker(*acc, GetDefaultRuntime());

    std::vector<GameObjectEntry> gameObjects;
    if (!walker.EnumerateGameObjects(gameObjects)) return out;

    out.reserve(gameObjects.size());
    for (const auto& go : gameObjects)
    {
        if (!go.nativeObject) continue;
        NativeGameObject ngo(go.nativeObject);
        std::string goName = ngo.GetName();
        if (goName.empty()) continue;
        if (ContainsAllSubstrings(goName, parts))
        {
            out.push_back(go.nativeObject);
        }
    }
    return out;
}

template <typename... Ts>
inline std::vector<std::uintptr_t> FindGameObjectsThroughNameContainsAll(Ts&&... parts)
{
    return FindGameObjectsThroughNameContainsAll(std::initializer_list<std::string_view>{std::string_view(parts)...});
}

inline std::vector<std::uintptr_t> FindComponentsOnGameObjectThroughClassNameContainsAll(
    std::uintptr_t gameObjectNative,
    std::initializer_list<std::string_view> parts)
{
    std::vector<std::uintptr_t> out;
    if (!gameObjectNative || parts.size() == 0) return out;

    NativeGameObject go(gameObjectNative);
    std::int32_t count = 0;
    if (!go.GetComponentCount(count) || count <= 0) return out;
    if (count > 1024) count = 1024;

    out.reserve(static_cast<std::size_t>(count));
    for (std::int32_t i = 0; i < count; ++i)
    {
        std::uintptr_t nativeComp = 0;
        if (!go.GetComponent(i, nativeComp) || !nativeComp) continue;

        std::uintptr_t managedComp = GetManagedFromComponent(nativeComp);
        if (!managedComp) continue;

        std::string className = GetManagedClassName(managedComp);
        if (className.empty()) continue;
        if (ContainsAllSubstrings(className, parts))
        {
            out.push_back(nativeComp);
        }
    }
    return out;
}

template <typename... Ts>
inline std::vector<std::uintptr_t> FindComponentsOnGameObjectThroughClassNameContainsAll(
    std::uintptr_t gameObjectNative,
    Ts&&... parts)
{
    return FindComponentsOnGameObjectThroughClassNameContainsAll(
        gameObjectNative,
        std::initializer_list<std::string_view>{std::string_view(parts)...});
}

inline std::vector<std::uintptr_t> FindComponentsThroughClassNameContainsAll(std::initializer_list<std::string_view> parts)
{
    std::vector<std::uintptr_t> out;
    if (parts.size() == 0) return out;

    auto comps = EnumerateComponents();
    out.reserve(comps.size());

    for (const auto& c : comps)
    {
        if (!c.nativeComponent) continue;
        if (!c.managedComponent) continue;

        std::string className = GetManagedClassName(c.managedComponent);
        if (className.empty()) continue;
        if (ContainsAllSubstrings(className, parts))
        {
            out.push_back(c.nativeComponent);
        }
    }
    return out;
}

template <typename... Ts>
inline std::vector<std::uintptr_t> FindComponentsThroughClassNameContainsAll(Ts&&... parts)
{
    return FindComponentsThroughClassNameContainsAll(std::initializer_list<std::string_view>{std::string_view(parts)...});
}

inline std::uintptr_t GetComponentThroughTypeId(std::uintptr_t gameObjectNative, std::int32_t typeId)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc || !gameObjectNative) return 0;

    std::uintptr_t pool = 0;
    if (!GetComponentPool(*acc, gameObjectNative, pool)) return 0;

    std::int32_t count = 0;
    if (!GetComponentCount(*acc, gameObjectNative, count)) return 0;
    if (count <= 0 || count > 1024) return 0;

    for (int i = 0; i < count; ++i)
    {
        std::int32_t typeIdValue = 0;
        if (!GetComponentSlotTypeId(*acc, pool, i, typeIdValue)) continue;
        if (typeIdValue != typeId) continue;

        std::uintptr_t nativeComp = 0;
        if (GetComponentSlotNative(*acc, pool, i, nativeComp)) return nativeComp;
    }
    return 0;
}

inline std::uintptr_t GetComponentThroughTypeName(std::uintptr_t gameObjectNative, const std::string& typeName)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc || !gameObjectNative) return 0;

    std::uintptr_t pool = 0;
    if (!GetComponentPool(*acc, gameObjectNative, pool)) return 0;

    std::int32_t count = 0;
    if (!GetComponentCount(*acc, gameObjectNative, count)) return 0;
    if (count <= 0 || count > 1024) return 0;

    RuntimeKind runtime = GetDefaultRuntime();

    for (int i = 0; i < count; ++i)
    {
        std::uintptr_t nativeComp = 0;
        if (!GetComponentSlotNative(*acc, pool, i, nativeComp)) continue;

        std::uintptr_t managedComp = 0;
        if (!GetManagedFromComponent(*acc, nativeComp, managedComp)) continue;

        TypeInfo info;
        if (GetManagedType(runtime, *acc, managedComp, info) && info.name == typeName)
        {
            return nativeComp;
        }
    }
    return 0;
}

inline std::uintptr_t FindGameObjectWithComponent(const std::string& typeName)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    GOMWalker walker(*acc, GetDefaultRuntime());

    std::vector<GameObjectEntry> gameObjects;
    if (!walker.EnumerateGameObjects(gameObjects)) return 0;

    for (const auto& go : gameObjects)
    {
        if (!go.nativeObject) continue;
        if (GetComponentThroughTypeName(go.nativeObject, typeName))
        {
            return go.nativeObject;
        }
    }
    return 0;
}

} // namespace UnityExternal
