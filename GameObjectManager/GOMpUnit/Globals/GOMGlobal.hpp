#pragma once

#include <cstdint>
#include <atomic>

#include "../../../Core/UnityExternalMemory.hpp"
#include "../../../Core/UnityExternalMemoryConfig.hpp"
#include "../../../Core/UnityExternalTypes.hpp"

namespace UnityExternal
{

inline std::atomic<std::uintptr_t> g_gomGlobal{0};
inline std::atomic<RuntimeKind> g_defaultRuntime{RuntimeKind::Mono};

inline void SetGOMGlobal(std::uintptr_t gomGlobal)
{
    g_gomGlobal.store(gomGlobal, std::memory_order_release);
}

inline void SetDefaultRuntime(RuntimeKind runtime)
{
    g_defaultRuntime.store(runtime, std::memory_order_release);
}

inline std::uintptr_t GetGOMGlobal()
{
    return g_gomGlobal.load(std::memory_order_acquire);
}

inline RuntimeKind GetDefaultRuntime()
{
    return g_defaultRuntime.load(std::memory_order_acquire);
}

} // namespace UnityExternal
