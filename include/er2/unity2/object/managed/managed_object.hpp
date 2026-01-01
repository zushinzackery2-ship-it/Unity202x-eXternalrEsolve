#pragma once

#include <cstdint>
#include <string>

#include "../../../core/types.hpp"
#include "../../../mem/memory_read.hpp"
#include "il2cpp_class.hpp"
#include "../../core/offsets.hpp"

namespace er2
{

struct TypeInfo
{
    std::string name;
    std::string namespaze;
};

/// <summary>

/// Il2CppManagedObjectHeader { void* klass; void* monitor; }
/// </summary>
inline bool ReadManagedObjectKlass(const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::uintptr_t& outKlass)
{
    outKlass = 0;

    if (!IsCanonicalUserPtr(managed))
    {
        return false;
    }

    
    std::uintptr_t klass = 0;
    if (!ReadPtr(mem, managed + off.managed_object_klass, klass))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(klass))
    {
        return false;
    }

    outKlass = klass;
    return true;
}

inline bool ReadManagedObjectTypeInfo(const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, TypeInfo& out)
{
    out = TypeInfo{};

    std::uintptr_t klass = 0;
    if (!ReadManagedObjectKlass(mem, managed, off, klass))
    {
        return false;
    }

    std::string ns;
    std::string cn;
    if (!ReadIl2CppClassName(mem, klass, off, ns, cn))
    {
        return false;
    }

    out.name = cn;
    out.namespaze = ns;
    return true;
}

inline bool ReadManagedObjectClassName(const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::string& outName)
{
    outName.clear();

    TypeInfo ti;
    if (!ReadManagedObjectTypeInfo(mem, managed, off, ti))
    {
        return false;
    }

    outName = ti.name;
    return true;
}

inline bool ReadManagedObjectNamespace(const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::string& outNs)
{
    outNs.clear();

    TypeInfo ti;
    if (!ReadManagedObjectTypeInfo(mem, managed, off, ti))
    {
        return false;
    }

    outNs = ti.namespaze;
    return true;
}

inline bool ReadManagedObjectClassFullName(const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::string& outFullName)
{
    outFullName.clear();

    std::uintptr_t klass = 0;
    if (!ReadManagedObjectKlass(mem, managed, off, klass))
    {
        return false;
    }

    std::string ns;
    std::string cn;
    if (!ReadIl2CppClassName(mem, klass, off, ns, cn))
    {
        return false;
    }

    outFullName = ns.empty() ? cn : (ns + "." + cn);
    return true;
}

} // namespace er2
