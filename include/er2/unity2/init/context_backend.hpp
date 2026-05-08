#pragma once

#include "context_types.hpp"

namespace er2
{

inline bool EnsureContextBackend()
{
    if (!g_ctx.backend)
    {
        g_ctx.backend = CreateDefaultContextBackend();
    }

    return static_cast<bool>(g_ctx.backend);
}

inline HANDLE GetProcessHandleFromBackend(const ContextBackendPtr& backend)
{
    if (!backend)
    {
        return nullptr;
    }

    return backend->GetProcessHandle();
}

inline bool GetContextModuleInfo(std::uint32_t pid, const wchar_t* moduleName, ModuleInfo& out)
{
    out = ModuleInfo{};
    if (!EnsureContextBackend())
    {
        return false;
    }

    return g_ctx.backend->GetModuleInfo(pid, moduleName, out);
}

inline void SetContextBackend(ContextBackendPtr backend)
{
    if (g_ctx.backend)
    {
        g_ctx.backend->Reset();
    }

    g_ctx = Context{};
    g_ctx.backend = backend ? backend : CreateDefaultContextBackend();
    g_ctx.process = GetProcessHandleFromBackend(g_ctx.backend);
}

inline void ResetContext()
{
    ContextBackendPtr backend = g_ctx.backend;
    if (!backend)
    {
        backend = CreateDefaultContextBackend();
    }

    if (backend)
    {
        backend->Reset();
    }

    g_ctx = Context{};
    g_ctx.backend = backend;
    g_ctx.process = GetProcessHandleFromBackend(g_ctx.backend);
}

inline bool InitSettings(std::uint32_t pid, ManagedBackend runtime)
{
    ResetContext();

    g_ctx.pid = pid;
    g_ctx.runtime = runtime;

    if (!EnsureContextBackend() || !g_ctx.backend->Attach(pid))
    {
        ResetContext();
        return false;
    }

    g_ctx.process = GetProcessHandleFromBackend(g_ctx.backend);

    if (!GetContextModuleInfo(pid, L"UnityPlayer.dll", g_ctx.unityPlayer))
    {
        ResetContext();
        return false;
    }

    g_ctx.unityPlayerRange.base = g_ctx.unityPlayer.base;
    g_ctx.unityPlayerRange.size = g_ctx.unityPlayer.size;

    return true;
}

inline bool InitBase(std::uint64_t gomSlotRva, std::uint64_t msIdToPointerSlotRva)
{
    if (!EnsureContextBackend() || !g_ctx.backend->IsAttached() || !g_ctx.unityPlayer.base)
    {
        return false;
    }

    g_ctx.gomGlobalSlotRva = gomSlotRva;
    g_ctx.gomGlobalSlotVa = g_ctx.unityPlayer.base + static_cast<std::uintptr_t>(gomSlotRva);

    g_ctx.msIdToPointerSlotRva = msIdToPointerSlotRva;
    g_ctx.msIdToPointerSlotVa = g_ctx.unityPlayer.base + static_cast<std::uintptr_t>(msIdToPointerSlotRva);

    return true;
}

} // namespace er2
