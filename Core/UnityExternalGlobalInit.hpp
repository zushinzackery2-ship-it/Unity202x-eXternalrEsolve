#pragma once

#include <cstdint>

#include "UnityExternalMemoryConfig.hpp"
#include "../GameObjectManager/GOMpUnit/Globals/GOMGlobal.hpp"

namespace UnityExternal
{

inline bool InitGlobalContext(const IMemoryAccessor* accessor, std::uint32_t pid)
{
    SetGlobalMemoryAccessor(accessor);
    SetTargetPid(pid);
    return accessor != nullptr && pid != 0;
}

inline bool InitGlobalContext(const IMemoryAccessor* accessor,
                             std::uint32_t pid,
                             RuntimeKind runtime,
                             std::uintptr_t gomGlobal)
{
    SetGlobalMemoryAccessor(accessor);
    SetTargetPid(pid);
    SetDefaultRuntime(runtime);
    SetGOMGlobal(gomGlobal);
    return accessor != nullptr && pid != 0 && gomGlobal != 0;
}

} // namespace UnityExternal
