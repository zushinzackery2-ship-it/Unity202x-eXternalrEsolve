#pragma once

#include <cstdint>
#include <string>

#include "../Globals/GOMGlobal.hpp"

namespace UnityExternal
{

inline bool GetManagedFromNative(const IMemoryAccessor& mem, std::uintptr_t nativeObject, std::uintptr_t& outManaged)
{
    outManaged = 0;
    if (!nativeObject) return false;
    return ReadPtr(mem, nativeObject + 0x28, outManaged) && outManaged != 0;
}

inline std::uintptr_t GetManagedFromNative(std::uintptr_t nativeObject)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetManagedFromNative(*acc, nativeObject, out);
    return out;
}

inline bool GetManagedFromComponent(const IMemoryAccessor& mem, std::uintptr_t nativeComponent, std::uintptr_t& outManaged)
{
    outManaged = 0;
    if (!nativeComponent) return false;
    return ReadPtr(mem, nativeComponent + 0x28, outManaged) && outManaged != 0;
}

inline std::uintptr_t GetManagedFromComponent(std::uintptr_t nativeComponent)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;
    std::uintptr_t out = 0;
    GetManagedFromComponent(*acc, nativeComponent, out);
    return out;
}

inline bool GetManagedType(std::uintptr_t managedObject, TypeInfo& out)
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return false;
    return GetManagedType(GetDefaultRuntime(), *acc, managedObject, out);
}

inline std::string GetManagedClassName(std::uintptr_t managedObject)
{
    TypeInfo info;
    if (!GetManagedType(managedObject, info)) return std::string();
    return info.name;
}

inline std::string GetManagedNamespace(std::uintptr_t managedObject)
{
    TypeInfo info;
    if (!GetManagedType(managedObject, info)) return std::string();
    return info.namespaze;
}

} // namespace UnityExternal
