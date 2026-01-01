#pragma once

#include <cstdint>
#include <string>

#include "../../../core/types.hpp"
#include "../../../mem/memory_read.hpp"
#include "il2cpp_class.hpp"
#include "mono_class.hpp"
#include "managed_backend.hpp"
#include "../../core/offsets.hpp"

namespace er2
{

struct TypeInfo
{
    std::string name;
    std::string namespaze;
};

/// <summary>
/// Reads the 'klass' field from the managed object header.
/// For IL2CPP, this is Il2CppClass*.
/// For Mono, this is MonoVTable*.
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

inline bool ReadManagedObjectTypeInfo(ManagedBackend runtime, const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, TypeInfo& out)
{
    out = TypeInfo{};

    std::uintptr_t klass = 0;
    // Note: For Mono, 'klass' here is actually the vtable pointer.
    if (!ReadManagedObjectKlass(mem, managed, off, klass))
    {
        return false;
    }

    std::string ns;
    std::string cn;
    bool success = false;

    if (runtime == ManagedBackend::Il2Cpp)
    {
        success = ReadIl2CppClassName(mem, klass, off, ns, cn);
    }
    else // Mono
    {
        success = ReadMonoClassName(mem, klass, off, ns, cn);
    }

    if (!success)
    {
        return false;
    }

    out.name = cn;
    out.namespaze = ns;
    return true;
}

inline bool ReadManagedObjectClassName(ManagedBackend runtime, const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::string& outName)
{
    outName.clear();

    TypeInfo ti;
    if (!ReadManagedObjectTypeInfo(runtime, mem, managed, off, ti))
    {
        return false;
    }

    outName = ti.name;
    return true;
}

inline bool ReadManagedObjectNamespace(ManagedBackend runtime, const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::string& outNs)
{
    outNs.clear();

    TypeInfo ti;
    if (!ReadManagedObjectTypeInfo(runtime, mem, managed, off, ti))
    {
        return false;
    }

    outNs = ti.namespaze;
    return true;
}

inline bool ReadManagedObjectClassFullName(ManagedBackend runtime, const IMemoryAccessor& mem, std::uintptr_t managed, const Offsets& off, std::string& outFullName)
{
    outFullName.clear();
    
    TypeInfo ti;
    if (!ReadManagedObjectTypeInfo(runtime, mem, managed, off, ti))
    {
        return false;
    }

    outFullName = ti.namespaze.empty() ? ti.name : (ti.namespaze + "." + ti.name);
    return true;
}

} // namespace er2
