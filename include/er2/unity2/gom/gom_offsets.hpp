#pragma once

#include <cstdint>

namespace er2
{

/// <summary>


/// </summary>
struct GomManagerOffsets
{
    std::uint32_t buckets_ptr = 0x00;   // Il2CppGameObjectManager.buckets
    std::uint32_t bucket_count = 0x08;  // Il2CppGameObjectManager.bucketCount
    std::uint32_t local_game_object_list_head = 0x28;
};

/// <summary>


/// </summary>
struct GomBucketOffsets
{
    std::uint32_t hash_mask = 0x00;     // Il2CppBucket.hashMask
    std::uint32_t info = 0x04;          // Il2CppBucket.info
    std::uint32_t key = 0x08;           
    std::uint32_t list_head = 0x10;     // Il2CppBucket.listHead
    std::uint32_t stride = 0x18;        // 24 bytes
};

/// <summary>


/// </summary>
struct GomListNodeOffsets
{
    std::uint32_t prev = 0x00;          // Il2CppGameObjectNode.pLast
    std::uint32_t next = 0x08;          // Il2CppGameObjectNode.pNext
    std::uint32_t native_object = 0x10; // Il2CppGameObjectNode.nativeObject
};

/// <summary>


/// </summary>
struct NativeGameObjectOffsets
{
    std::uint32_t managed = 0x28;           // Il2CppNativeGameObject.managedObject
    std::uint32_t component_pool = 0x30;    // Il2CppNativeGameObject.componentPool
    std::uint32_t component_count = 0x40;   // Il2CppNativeGameObject.componentCount
    std::uint32_t tag_raw = 0x54;           
    std::uint32_t name_ptr = 0x60;          // Il2CppNativeGameObject.namePtr
};

/// <summary>


/// </summary>
struct NativeComponentOffsets
{
    std::uint32_t managed = 0x28;       // Il2CppNativeComponent.managedComponent
    std::uint32_t game_object = 0x30;   // Il2CppNativeComponent.gameObject
    std::uint32_t enabled = 0x38;
};

/// <summary>


/// </summary>
struct ComponentPoolOffsets
{
    std::uint32_t slot_stride = 0x10;   // Il2CppComponentSlot stride
    std::uint32_t slot_type_id = 0x00;  // Il2CppComponentSlot.typeId
    std::uint32_t slot_native = 0x08;   // Il2CppComponentSlot.nativeComponent
};

/// <summary>

/// </summary>
struct GomOffsets
{
    GomManagerOffsets manager;
    GomBucketOffsets bucket;
    GomListNodeOffsets node;
    NativeGameObjectOffsets game_object;
    NativeComponentOffsets component;
    ComponentPoolOffsets pool;
};

} // namespace er2
