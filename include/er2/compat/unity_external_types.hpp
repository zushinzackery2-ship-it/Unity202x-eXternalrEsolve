#pragma once




#include "../unity2/core/runtime_types.hpp"
#include "../mem/memory_read.hpp"

namespace UnityExternal
{
    
    using RuntimeKind = er2::unity2::core::RuntimeKind;
    using TypeInfo = er2::unity2::core::TypeInfo;
    using IMemoryAccessor = er2::IMemoryAccessor;

    
    using er2::ReadValue;
    using er2::ReadPtr;
    using er2::ReadInt32;
    using er2::ReadCString;
    using er2::WriteValue;
    using er2::unity2::core::GetManagedType;
    using er2::unity2::core::GetManagedClassName;
    using er2::unity2::core::GetManagedNamespace;

} // namespace UnityExternal