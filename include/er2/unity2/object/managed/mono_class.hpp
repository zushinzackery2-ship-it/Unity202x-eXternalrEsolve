#pragma once

#include <cstdint>
#include <string>

#include "../../../core/types.hpp"
#include "../../../mem/memory_read.hpp"
#include "../../core/offsets.hpp"

namespace er2
{

inline bool ReadMonoClassName(const IMemoryAccessor& mem, std::uintptr_t vtable, const Offsets& off, std::string& outNamespace, std::string& outClassName)
{
    outNamespace.clear();
    outClassName.clear();

    if (!IsCanonicalUserPtr(vtable))
    {
        return false;
    }

    // MonoVTable.klass is at 0x0
    std::uintptr_t klass = 0;
    if (!ReadPtr(mem, vtable, klass))
    {
        return false;
    }

    if (!IsCanonicalUserPtr(klass))
    {
        return false;
    }

    // MonoClass.name is at 0x48 (usually const char*)
    std::uintptr_t namePtr = 0;
    if (!ReadPtr(mem, klass + off.mono_class_name, namePtr))
    {
        return false;
    }
    
    if (!IsCanonicalUserPtr(namePtr))
    {
        return false;
    }

    if (!ReadCString(mem, namePtr, outClassName))
    {
        return false;
    }

    // MonoClass.name_space is at 0x50
    std::uintptr_t nsPtr = 0;
    if (ReadPtr(mem, klass + off.mono_class_namespace, nsPtr) && IsCanonicalUserPtr(nsPtr))
    {
        ReadCString(mem, nsPtr, outNamespace);
    }
    
    // Namespace can be null, handled by ReadCString
    return true;
}

inline bool IsMonoClassOrParent(const IMemoryAccessor& mem, std::uintptr_t vtable, const Offsets& off, const char* targetNs, const char* targetName, int maxDepth = 10)
{
    if (!IsCanonicalUserPtr(vtable) || !targetName)
    {
        return false;
    }

    std::uintptr_t currentKlass = 0;
    if (!ReadPtr(mem, vtable, currentKlass)) return false;

    for (int i = 0; i < maxDepth && IsCanonicalUserPtr(currentKlass); ++i)
    {
        std::string ns;
        std::string cn;
        
        // Read directly from Class, not VTable
        std::uintptr_t namePtr = 0;
        if (ReadPtr(mem, currentKlass + off.mono_class_name, namePtr) && IsCanonicalUserPtr(namePtr)) {
             ReadCString(mem, namePtr, cn);
        }
        
        std::uintptr_t nsPtr = 0;
        if (ReadPtr(mem, currentKlass + off.mono_class_namespace, nsPtr) && IsCanonicalUserPtr(nsPtr)) {
             ReadCString(mem, nsPtr, ns);
        }

        bool nsMatch = (targetNs == nullptr || targetNs[0] == '\0' || ns == targetNs);
        bool cnMatch = (cn == targetName);

        if (nsMatch && cnMatch)
        {
            return true;
        }

        std::uintptr_t parentKlass = 0;
        if (!ReadPtr(mem, currentKlass + off.mono_class_parent, parentKlass))
        {
            break;
        }
        currentKlass = parentKlass;
    }
    return false;
}

} // namespace er2
