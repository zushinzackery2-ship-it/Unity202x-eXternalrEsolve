#pragma once

#include <windows.h>

#include <cstdint>
#include <atomic>

#include "../../MemoryAccessor/Resolver/WinAPIResolver.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

inline std::atomic<std::uintptr_t> g_metadataModuleBase{0};
inline std::atomic<std::uint32_t> g_metadataRequiredVersion{0};

}

inline void SetMetadataTargetModule(std::uintptr_t moduleBase)
{
    detail_metadata::g_metadataModuleBase.store(moduleBase, std::memory_order_release);
}

inline std::uintptr_t GetMetadataTargetModule()
{
    return detail_metadata::g_metadataModuleBase.load(std::memory_order_acquire);
}

inline bool SetMetadataTarget(DWORD pid, const wchar_t* moduleName)
{
    if (!moduleName || !moduleName[0]) return false;
    std::uintptr_t moduleBase = FindModuleBase(pid, moduleName);
    if (!moduleBase) return false;
    SetMetadataTargetModule(moduleBase);
    return true;
}

inline void SetMetadataTargetVersion(std::uint32_t version)
{
    detail_metadata::g_metadataRequiredVersion.store(version, std::memory_order_release);
}

inline std::uint32_t GetMetadataTargetVersion()
{
    return detail_metadata::g_metadataRequiredVersion.load(std::memory_order_acquire);
}

}
