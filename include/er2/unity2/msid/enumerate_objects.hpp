#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "../../mem/memory_read.hpp"
#include "../object/managed/il2cpp_class.hpp"
#include "../object/managed/mono_class.hpp"
#include "../object/managed/managed_backend.hpp"
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
    Other = 3,
};

struct ObjectInfo
{
    std::uintptr_t native = 0;
    std::uint32_t instanceId = 0;
    std::string objectName;
    std::string typeFullName;
    ObjectKind kind = ObjectKind::Other;
};

struct EnumerateOptions
{
    bool onlyGameObject = true;
    bool onlyScriptableObject = true;
    std::string filterLower;
};

inline bool MatchFilterLower(const std::string& s, const std::string& filterLower)
{
    if (filterLower.empty()) return true;
    std::string tmp;
    tmp.reserve(s.size());
    for (char c : s)
    {
        if (c >= 'A' && c <= 'Z') tmp.push_back(static_cast<char>(c - 'A' + 'a'));
        else tmp.push_back(c);
    }
    return tmp.find(filterLower) != std::string::npos;
}

inline bool EnumerateMsIdToPointerObjects(
    ManagedBackend runtime,
    const IMemoryAccessor& mem,
    std::uintptr_t msIdToPointerAddr,
    const Offsets& off,
    const UnityPlayerRange& unityPlayer,
    const EnumerateOptions& opt,
    const std::function<void(const ObjectInfo&)>& cb)
{
    MsIdToPointerSet set;
    if (!ReadMsIdToPointerSet(mem, msIdToPointerAddr, set)) return false;

    std::uintptr_t entriesBase = 0;
    std::uint32_t capacity = 0;
    std::uint32_t count = 0;
    if (!ReadMsIdEntriesHeader(mem, set, entriesBase, capacity, count)) return false;

    const std::uint32_t kChunkSize = 65536; 
    std::vector<std::uint8_t> buffer;
    buffer.resize(kChunkSize * kMsIdEntryStride);

    const std::uint32_t kObjHeaderSize = 0x60;
    std::uint8_t objBuffer[kObjHeaderSize];

    const std::uint32_t kNameBufferSize = 128;
    std::uint8_t nameBuffer[kNameBufferSize];

    struct CachedClassInfo {
        bool valid;
        bool isGo;
        bool isSo;
        std::string ns;
        std::string cn;
    };
    std::unordered_map<std::uintptr_t, CachedClassInfo> classCache;

    for (std::uint32_t i = 0; i < capacity; i += kChunkSize)
    {
        std::uint32_t currentChunkCount = capacity - i;
        if (currentChunkCount > kChunkSize) currentChunkCount = kChunkSize;

        const std::uintptr_t chunkStart = entriesBase + static_cast<std::uintptr_t>(i) * kMsIdEntryStride;
        
        if (!mem.Read(chunkStart, buffer.data(), currentChunkCount * kMsIdEntryStride)) continue;

        for (std::uint32_t j = 0; j < currentChunkCount; ++j)
        {
            const MsIdToPointerEntryRaw* raw = reinterpret_cast<const MsIdToPointerEntryRaw*>(buffer.data() + j * kMsIdEntryStride);

            if (raw->hashMask >= 0xFFFFFFFEu) continue;
            if (raw->key == 0) continue;

            const std::uintptr_t obj = raw->object;
            
            if (!mem.Read(obj, objBuffer, kObjHeaderSize)) continue;

            std::uintptr_t vtable = *reinterpret_cast<std::uintptr_t*>(objBuffer);
            if (!IsCanonicalUserPtr(vtable)) continue;
            
            if (vtable < unityPlayer.base || vtable >= (unityPlayer.base + unityPlayer.size)) continue;

            if (off.unity_object_managed_ptr >= kObjHeaderSize) continue;
            std::uintptr_t managed = *reinterpret_cast<std::uintptr_t*>(objBuffer + off.unity_object_managed_ptr);
            if (!IsCanonicalUserPtr(managed)) continue;

            std::uintptr_t klass = 0;
            if (!ReadPtr(mem, managed, klass)) continue; 
            
            bool isGo = false;
            bool isSo = false;
            std::string ns;
            std::string cn;

            auto it = classCache.find(klass);
            if (it != classCache.end()) 
            {
                if (!it->second.valid) continue;
                isGo = it->second.isGo;
                isSo = it->second.isSo;
                ns = it->second.ns;
                cn = it->second.cn;
            }
            else
            {
                CachedClassInfo cacheEntry;
                cacheEntry.valid = false;

                if (runtime == ManagedBackend::Il2Cpp)
                {
                    cacheEntry.isGo = IsClassOrParent(mem, klass, off, "UnityEngine", "GameObject");
                    cacheEntry.isSo = IsClassOrParent(mem, klass, off, "UnityEngine", "ScriptableObject");
                }
                else
                {
                    cacheEntry.isGo = IsMonoClassOrParent(mem, klass, off, "UnityEngine", "GameObject");
                    cacheEntry.isSo = IsMonoClassOrParent(mem, klass, off, "UnityEngine", "ScriptableObject");
                }

                // Always read class name regardless of type
                bool nameSuccess = false;
                if (runtime == ManagedBackend::Il2Cpp)
                    nameSuccess = ReadIl2CppClassName(mem, klass, off, cacheEntry.ns, cacheEntry.cn);
                else
                    nameSuccess = ReadMonoClassName(mem, klass, off, cacheEntry.ns, cacheEntry.cn);
                
                if (nameSuccess)
                    cacheEntry.valid = true;

                classCache[klass] = cacheEntry;

                if (!cacheEntry.valid) continue;
                isGo = cacheEntry.isGo;
                isSo = cacheEntry.isSo;
                ns = cacheEntry.ns;
                cn = cacheEntry.cn;
            }

            // Determine ObjectKind
            ObjectKind kind;
            if (isGo) kind = ObjectKind::GameObject;
            else if (isSo) kind = ObjectKind::ScriptableObject;
            else kind = ObjectKind::Other;

            // Filtering logic: 
            // If onlyGameObject is true, only include GameObjects
            // If onlyScriptableObject is true, only include ScriptableObjects
            // If both are false, include everything (for FindObjectsOfTypeAll)
            if (opt.onlyGameObject && !opt.onlyScriptableObject)
            {
                if (kind != ObjectKind::GameObject) continue;
            }
            else if (!opt.onlyGameObject && opt.onlyScriptableObject)
            {
                if (kind != ObjectKind::ScriptableObject) continue;
            }
            else if (opt.onlyGameObject && opt.onlyScriptableObject)
            {
                if (kind != ObjectKind::GameObject && kind != ObjectKind::ScriptableObject) continue;
            }
            // else: both false, include everything

            std::string name;
            std::uintptr_t namePtr = 0;

            if (kind == ObjectKind::GameObject) {
                 if (off.game_object_name_ptr < kObjHeaderSize) {
                      namePtr = *reinterpret_cast<std::uintptr_t*>(objBuffer + off.game_object_name_ptr);
                 } else {
                      ReadPtr(mem, obj + off.game_object_name_ptr, namePtr);
                 }
            } else if (kind == ObjectKind::ScriptableObject) {
                 ReadScriptableObjectName(mem, obj, off, name);
            }
            // For Other types, name stays empty (or we could try to read it)

            if (kind == ObjectKind::GameObject && namePtr != 0)
            {
                 if (mem.Read(namePtr, nameBuffer, kNameBufferSize))
                 {
                     nameBuffer[kNameBufferSize - 1] = '\0';
                     name = reinterpret_cast<char*>(nameBuffer);
                 }
                 else
                 {
                     ReadCString(mem, namePtr, name);
                 }
            }

            std::string full = ns.empty() ? cn : (ns + "." + cn);
            if (!MatchFilterLower(name, opt.filterLower) && !MatchFilterLower(full, opt.filterLower)) continue;

            ObjectInfo info;
            info.native = obj;
            info.instanceId = raw->key;
            info.objectName = name;
            info.typeFullName = full;
            info.kind = kind;
            cb(info);
        }
    }

    return true;
}

} // namespace er2
