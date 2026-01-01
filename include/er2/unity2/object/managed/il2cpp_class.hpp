#pragma once

#include <cstdint>
#include <string>

#include "../../../core/types.hpp"
#include "../../../mem/memory_read.hpp"
#include "../../core/offsets.hpp"

namespace er2
{

inline bool ReadIl2CppClassName(const IMemoryAccessor& mem, std::uintptr_t klass, const Offsets& off, std::string& outNamespace, std::string& outClassName)
{
    outNamespace.clear();
    outClassName.clear();

    if (!IsCanonicalUserPtr(klass))
    {
        return false;
    }

    std::uintptr_t namePtr = 0;
    if (!ReadPtr(mem, klass + off.il2cppclass_name_ptr, namePtr))
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

    std::uintptr_t nsPtr = 0;
    if (ReadPtr(mem, klass + off.il2cppclass_namespace_ptr, nsPtr) && IsCanonicalUserPtr(nsPtr))
    {
        ReadCString(mem, nsPtr, outNamespace);
    }

    return true;
}

inline bool IsClassOrParent(const IMemoryAccessor& mem, std::uintptr_t klass, const Offsets& off, const char* targetNs, const char* targetName, int maxDepth = 10)
{
    if (!IsCanonicalUserPtr(klass) || !targetName)
    {
        return false;
    }

    std::uintptr_t current = klass;
    for (int i = 0; i < maxDepth && IsCanonicalUserPtr(current); ++i)
    {
        std::string ns;
        std::string cn;
        if (!ReadIl2CppClassName(mem, current, off, ns, cn))
        {
            break;
        }

        bool nsMatch = (targetNs == nullptr || targetNs[0] == '\0' || ns == targetNs);
        bool cnMatch = (cn == targetName);

        if (nsMatch && cnMatch)
        {
            return true;
        }

        std::uintptr_t parent = 0;
        if (!ReadPtr(mem, current + off.il2cppclass_parent, parent))
        {
            break;
        }

        current = parent;
    }

    return false;
}

} // namespace er2
