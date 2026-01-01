#pragma once

#include "../../core/types.hpp"
#include "../../mem/memory_accessor.hpp"
#include "../../mem/memory_read.hpp"
#include <string>

namespace er2::unity2::core
{
    /// <summary>
    
    /// </summary>
    enum class RuntimeKind : i32
    {
        Il2Cpp,
        Mono
    };

    /// <summary>
    
    /// </summary>
    struct TypeInfo
    {
        std::string name;
        std::string namespaze;
    };

    namespace detail
    {
        /// <summary>
        
        /// </summary>
        struct Il2CppManagedObjectHeader
        {
            uptr klass;
        };

        /// <summary>
        
        /// </summary>
        struct Il2CppClassInternal
        {
            unsigned char pad[0x10];  
            uptr name;
            uptr namespaze;
        };

        /// <summary>
        
        /// </summary>
        struct MonoManagedObjectHeader
        {
            uptr vtable;
        };

        /// <summary>
        
        /// </summary>
        struct MonoVTableInternal
        {
            uptr klass;
        };

        /// <summary>
        
        /// </summary>
        struct MonoClassInternal
        {
            unsigned char pad[0x48];  
            uptr name;
            uptr namespaze;
        };

        /// <summary>
        
        /// </summary>
        inline bool GetIl2CppType(const IMemoryAccessor& mem, uptr managedObj, TypeInfo& out)
        {
            Il2CppManagedObjectHeader header{};
            if (!ReadValue(mem, managedObj, header))
            {
                return false;
            }
            if (!header.klass)
            {
                return false;
            }

            Il2CppClassInternal klass{};
            if (!ReadValue(mem, header.klass, klass))
            {
                return false;
            }

            std::string name;
            std::string ns;
            if (!ReadCString(mem, klass.name, name))
            {
                return false;
            }
            if (klass.namespaze)
            {
                if (!ReadCString(mem, klass.namespaze, ns))
                {
                    return false;
                }
            }

            out.name = name;
            out.namespaze = ns;
            return true;
        }

        /// <summary>
        
        /// </summary>
        inline bool GetMonoType(const IMemoryAccessor& mem, uptr managedObj, TypeInfo& out)
        {
            MonoManagedObjectHeader header{};
            if (!ReadValue(mem, managedObj, header))
            {
                return false;
            }
            if (!header.vtable)
            {
                return false;
            }

            MonoVTableInternal vtable{};
            if (!ReadValue(mem, header.vtable, vtable))
            {
                return false;
            }
            if (!vtable.klass)
            {
                return false;
            }

            MonoClassInternal klass{};
            if (!ReadValue(mem, vtable.klass, klass))
            {
                return false;
            }

            std::string name;
            std::string ns;
            if (!ReadCString(mem, klass.name, name))
            {
                return false;
            }
            if (klass.namespaze)
            {
                if (!ReadCString(mem, klass.namespaze, ns))
                {
                    return false;
                }
            }

            out.name = name;
            out.namespaze = ns;
            return true;
        }

    } // namespace detail

    /// <summary>
    
    /// </summary>
    inline bool GetManagedType(RuntimeKind runtime, const IMemoryAccessor& mem, uptr managedObject, TypeInfo& out)
    {
        out = TypeInfo{};
        if (!managedObject)
        {
            return false;
        }

        if (runtime == RuntimeKind::Il2Cpp)
        {
            return detail::GetIl2CppType(mem, managedObject, out);
        }

        return detail::GetMonoType(mem, managedObject, out);
    }

    /// <summary>
    
    /// </summary>
    inline bool GetManagedClassName(RuntimeKind runtime, const IMemoryAccessor& mem, uptr managedObject, std::string& out)
    {
        out.clear();
        TypeInfo info;
        if (!GetManagedType(runtime, mem, managedObject, info))
        {
            return false;
        }
        out = info.name;
        return true;
    }

    /// <summary>
    
    /// </summary>
    inline bool GetManagedNamespace(RuntimeKind runtime, const IMemoryAccessor& mem, uptr managedObject, std::string& out)
    {
        out.clear();
        TypeInfo info;
        if (!GetManagedType(runtime, mem, managedObject, info))
        {
            return false;
        }
        out = info.namespaze;
        return true;
    }

} // namespace er2::unity2::core