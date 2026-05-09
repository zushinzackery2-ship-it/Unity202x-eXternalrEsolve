#pragma once

#include <cstring>
#include <cstdint>
#include <unordered_set>
#include <vector>
#include "../../mem/memory_read.hpp"
#include "gom_bucket.hpp"
#include "gom_list_node.hpp"
#include "gom_offsets.hpp"
#include "gom_pool.hpp"
#include "gom_walker_types.hpp"
namespace er2
{
inline bool GetManagedFromNative(const IMemoryAccessor& mem, std::uintptr_t nativeObject, const GomOffsets& off, std::uintptr_t& outManaged)
{
    outManaged = 0;
    if (!nativeObject)
    {
        return false;
    }

    return ReadPtr(mem, nativeObject + off.game_object.managed, outManaged) && outManaged != 0;
}

inline bool GetManagedFromComponent(const IMemoryAccessor& mem, std::uintptr_t nativeComponent, const GomOffsets& off, std::uintptr_t& outManaged)
{
    outManaged = 0;
    if (!nativeComponent)
    {
        return false;
    }

    return ReadPtr(mem, nativeComponent + off.component.managed, outManaged) && outManaged != 0;
}

/// <summary>
/// </summary>

inline bool EnumerateGameObjects(const IMemoryAccessor& mem, std::uintptr_t gomGlobalSlot, const GomOffsets& off, std::vector<GameObjectEntry>& out)
{
    out.clear();
    std::uintptr_t managerAddress = 0;
    if (!GetGomManagerFromGlobalSlot(mem, gomGlobalSlot, managerAddress))
    {
        return false;
    }

    std::unordered_set<std::uintptr_t> visitedNodes;
    const int kMaxNodesPerBucket = 200000;  // Increased from 10000 to handle large scenes
    std::vector<std::uintptr_t> buckets;
    if (GetAllLinkedBucketsFromManager(mem, managerAddress, off, buckets) && !buckets.empty())
    {
        for (std::uintptr_t bucketPtr : buckets)
        {
            std::uintptr_t listHead = 0;
            if (!GetBucketListHead(mem, bucketPtr, off, listHead) || !listHead)
            {
                continue;
            }

            std::uintptr_t node = listHead;
            int nodeCount = 0;
            do
            {
                if (visitedNodes.find(node) != visitedNodes.end())
                {
                    break;
                }

                visitedNodes.insert(node);
                std::uint8_t nodeBuf[24] = {};
                const std::size_t nodeReadSize = off.node.native_object + sizeof(std::uintptr_t);
                if (!mem.Read(node, nodeBuf, nodeReadSize < sizeof(nodeBuf) ? nodeReadSize : sizeof(nodeBuf)))
                {
                    break;
                }

                std::uintptr_t nativeObject = 0;
                std::memcpy(&nativeObject, nodeBuf + off.node.native_object, sizeof(nativeObject));
                if (nativeObject)
                {
                    std::uintptr_t managedObject = 0;
                    GetManagedFromNative(mem, nativeObject, off, managedObject);
                    GameObjectEntry e;
                    e.node = node;
                    e.nativeObject = nativeObject;
                    e.managedObject = managedObject;
                    out.push_back(e);
                }

                std::uintptr_t next = 0;
                std::memcpy(&next, nodeBuf + off.node.next, sizeof(next));
                if (!next || next == listHead)
                {
                    break;
                }

                node = next;
                ++nodeCount;
            } while (nodeCount < kMaxNodesPerBucket);
        }
    }

    std::uintptr_t localListHead = 0;
    if (GetGomLocalGameObjectListHead(mem, managerAddress, off, localListHead) && localListHead)
    {
        std::uintptr_t node = localListHead;
        int nodeCount = 0;
        do
        {
            if (visitedNodes.find(node) != visitedNodes.end())
            {
                break;
            }

            visitedNodes.insert(node);
            std::uint8_t nodeBuf[24] = {};
            const std::size_t nodeReadSize = off.node.native_object + sizeof(std::uintptr_t);
            if (!mem.Read(node, nodeBuf, nodeReadSize < sizeof(nodeBuf) ? nodeReadSize : sizeof(nodeBuf)))
            {
                break;
            }

            std::uintptr_t nativeObject = 0;
            std::memcpy(&nativeObject, nodeBuf + off.node.native_object, sizeof(nativeObject));
            if (nativeObject)
            {
                std::uintptr_t managedObject = 0;
                GetManagedFromNative(mem, nativeObject, off, managedObject);
                GameObjectEntry e;
                e.node = node;
                e.nativeObject = nativeObject;
                e.managedObject = managedObject;
                out.push_back(e);
            }

            std::uintptr_t next = 0;
            std::memcpy(&next, nodeBuf + off.node.next, sizeof(next));
            if (!next || next == localListHead)
            {
                break;
            }

            node = next;
            ++nodeCount;
        } while (nodeCount < kMaxNodesPerBucket);
    }

    return !out.empty();
}

} // namespace er2
