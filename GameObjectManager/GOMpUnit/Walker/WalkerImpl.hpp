#pragma once

#include <vector>

#include "GOMWalker.hpp"
#include "../Globals/GOMGlobal.hpp"
#include "../Manager/Manager.hpp"
#include "../Bucket/Bucket.hpp"
#include "../ListNode/ListNode.hpp"
#include "../Pool/Pool.hpp"
#include "../Managed/Managed.hpp"

namespace UnityExternal
{

inline bool GetAllLinkedBuckets(const IMemoryAccessor& mem,
                                RuntimeKind runtime,
                                std::uintptr_t gomGlobal,
                                std::vector<std::uintptr_t>& outBuckets)
{
    outBuckets.clear();
    if (!gomGlobal) return false;
    (void)runtime;

    std::uintptr_t managerAddress = 0;
    if (!GetGOMManagerFromGlobal(mem, gomGlobal, managerAddress)) return false;

    std::uintptr_t buckets = 0;
    if (!GetGOMBucketsPtr(mem, managerAddress, buckets)) return false;

    std::int32_t bucketCount = 0;
    if (!GetGOMBucketCount(mem, managerAddress, bucketCount) || bucketCount <= 0 || bucketCount > 0x100000) return false;

    const std::uintptr_t bucketStride = 24;

    for (std::int32_t bi = 0; bi < bucketCount; ++bi)
    {
        std::uintptr_t bucketPtr = buckets + static_cast<std::uintptr_t>(bi) * bucketStride;

        if (!IsBucketHashmaskKeyConsistent(mem, bucketPtr)) continue;

        std::uintptr_t listHead = 0;
        if (!GetBucketValue(mem, bucketPtr, listHead)) continue;

        std::uintptr_t node = 0;
        if (!GetListNodeFirst(mem, listHead, node)) continue;

        outBuckets.push_back(bucketPtr);
    }

    return !outBuckets.empty();
}

inline bool GOMWalker::EnumerateGameObjectsImpl(std::uintptr_t /*managerAddress*/, std::vector<GameObjectEntry>& out) const {
    out.clear();

    std::uintptr_t gomGlobal = GetGOMGlobal();
    if (!gomGlobal) return false;

    std::vector<std::uintptr_t> buckets;
    if (!GetAllLinkedBuckets(mem_, runtime_, gomGlobal, buckets) || buckets.empty())
    {
        return false;
    }

    const std::size_t kMaxObjects = 1000000;

    for (std::uintptr_t bucketPtr : buckets)
    {
        std::uintptr_t listHead = 0;
        if (!GetBucketValue(mem_, bucketPtr, listHead)) continue;

        std::uintptr_t node = 0;
        if (!GetListNodeFirst(mem_, listHead, node)) continue;

        for (std::size_t i = 0; node && i < kMaxObjects; ++i)
        {
            std::uintptr_t nativeObject = 0;
            std::uintptr_t managedObject = 0;
            std::uintptr_t next = 0;

            if (!GetListNodeNative(mem_, node, nativeObject)) break;
            if (nativeObject) GetManagedFromNative(mem_, nativeObject, managedObject);

            if (nativeObject || managedObject)
            {
                out.push_back({node, nativeObject, managedObject});
            }

            if (!GetListNodeNext(mem_, node, next) || !next || next == listHead) break;
            node = next;
        }
    }

    return !out.empty();
}

inline bool GOMWalker::EnumerateGameObjects(std::vector<GameObjectEntry>& out) const {
    std::uintptr_t gomGlobal = GetGOMGlobal();
    if (!gomGlobal) return false;

    std::uintptr_t managerAddress = 0;
    if (!ReadPtr(mem_, gomGlobal, managerAddress) || !managerAddress) return false;

    return EnumerateGameObjectsImpl(managerAddress, out);
}

inline bool GOMWalker::EnumerateComponentsImpl(std::uintptr_t managerAddress, std::vector<ComponentEntry>& out) const {
    out.clear();

    std::vector<GameObjectEntry> gameObjects;
    if (!EnumerateGameObjectsImpl(managerAddress, gameObjects) || gameObjects.empty()) return false;

    const int kMaxComponentsPerObject = 1024;

    for (const auto& info : gameObjects)
    {
        if (!info.nativeObject) continue;

        std::uintptr_t componentPool = 0;
        if (!GetComponentPool(mem_, info.nativeObject, componentPool) || !componentPool) continue;

        std::int32_t componentCount = 0;
        if (!GetComponentCount(mem_, info.nativeObject, componentCount)) continue;
        if (componentCount <= 0 || componentCount > kMaxComponentsPerObject) continue;

        for (int i = 0; i < componentCount; ++i)
        {
            std::uintptr_t nativeComponent = 0;
            if (!GetComponentSlotNative(mem_, componentPool, i, nativeComponent)) continue;

            std::uintptr_t managedComponent = 0;
            GetManagedFromComponent(mem_, nativeComponent, managedComponent);

            out.push_back({nativeComponent, managedComponent});
        }
    }

    return true;
}

inline bool GOMWalker::EnumerateComponents(std::vector<ComponentEntry>& out) const {
    std::uintptr_t gomGlobal = GetGOMGlobal();
    if (!gomGlobal) return false;

    std::uintptr_t managerAddress = 0;
    if (!ReadPtr(mem_, gomGlobal, managerAddress) || !managerAddress) return false;

    return EnumerateComponentsImpl(managerAddress, out);
}

// Camera view-projection matrix (+0x100)
inline bool GetCameraViewProjMatrix(const IMemoryAccessor& mem, std::uintptr_t nativeCamera, float outMatrix[16])
{
    if (!nativeCamera || !outMatrix) return false;
    return mem.Read(nativeCamera + 0x100u, outMatrix, 64);
}

} // namespace UnityExternal
