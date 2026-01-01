#pragma once

#include <Windows.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "../../os/win/win_memory_accessor.hpp"
#include "../../os/win/win_module.hpp"
#include "../../os/win/win_process.hpp"
#include "../../os/win/win_window.hpp"

#include "../../mem/memory_read.hpp"

#include "../object/managed/managed_backend.hpp"
#include "../object/native/native_object.hpp"

#include "../core/offsets.hpp"

#include "../gom/gom_offsets.hpp"
#include "../gom/gom_scan.hpp"

#include "../camera/camera.hpp"
#include "../transform/transform.hpp"

#include "../msid/msid_scan.hpp"

namespace er2
{

struct Context
{
    std::uint32_t pid = 0;
    HANDLE process = nullptr;

    ManagedBackend runtime = ManagedBackend::Mono;

    ModuleInfo unityPlayer;
    UnityPlayerRange unityPlayerRange;

    Offsets off;
    GomOffsets gomOff;
    CameraOffsets camOff;
    TransformOffsets transformOff;

    std::uintptr_t gomGlobalSlotVa = 0;
    std::uint64_t gomGlobalSlotRva = 0;

    std::uintptr_t msIdToPointerSlotVa = 0;
    std::uint64_t msIdToPointerSlotRva = 0;
};

inline Context g_ctx;

inline void ResetContext()
{
    if (g_ctx.process)
    {
        CloseHandle(g_ctx.process);
        g_ctx.process = nullptr;
    }

    g_ctx = Context{};
}

inline bool InitSettings(std::uint32_t pid, ManagedBackend runtime)
{
    ResetContext();

    g_ctx.pid = pid;
    g_ctx.runtime = runtime;

    g_ctx.process = OpenProcessForRead(pid);
    if (!g_ctx.process)
    {
        ResetContext();
        return false;
    }

    if (!GetRemoteModuleInfo(pid, L"UnityPlayer.dll", g_ctx.unityPlayer))
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
    if (!g_ctx.process || !g_ctx.unityPlayer.base)
    {
        return false;
    }

    g_ctx.gomGlobalSlotRva = gomSlotRva;
    g_ctx.gomGlobalSlotVa = g_ctx.unityPlayer.base + static_cast<std::uintptr_t>(gomSlotRva);

    g_ctx.msIdToPointerSlotRva = msIdToPointerSlotRva;
    g_ctx.msIdToPointerSlotVa = g_ctx.unityPlayer.base + static_cast<std::uintptr_t>(msIdToPointerSlotRva);

    return g_ctx.gomGlobalSlotVa != 0 && g_ctx.msIdToPointerSlotVa != 0;
}

inline bool AutoInit()
{
    const std::vector<std::uint32_t> pids = FindUnityWndClassPids();
    if (pids.empty())
    {
        return false;
    }

    for (const std::uint32_t pid : pids)
    {
        ModuleInfo up;
        if (!GetRemoteModuleInfo(pid, L"UnityPlayer.dll", up))
        {
            continue;
        }

        ModuleInfo ga;
        const bool isIl2Cpp = GetRemoteModuleInfo(pid, L"GameAssembly.dll", ga);
        const ManagedBackend runtime = isIl2Cpp ? ManagedBackend::Il2Cpp : ManagedBackend::Mono;

        if (!InitSettings(pid, runtime))
        {
            continue;
        }

        WinApiMemoryAccessor mem(g_ctx.process);

        std::uint64_t gomSlotRva = 0;
        if (!FindGomGlobalSlotRvaByScan(mem, g_ctx.unityPlayer.base, g_ctx.gomOff, gomSlotRva))
        {
            ResetContext();
            continue;
        }

        g_ctx.gomGlobalSlotRva = gomSlotRva;
        g_ctx.gomGlobalSlotVa = g_ctx.unityPlayer.base + static_cast<std::uintptr_t>(gomSlotRva);

        // MSID 扫描可选，失败不影响 GOM 功能
        std::uintptr_t msIdSlotVa = 0;
        if (FindMsIdToPointerSlotVaByScan(mem, g_ctx.unityPlayer, g_ctx.gomOff, msIdSlotVa, nullptr))
        {
            g_ctx.msIdToPointerSlotVa = msIdSlotVa;
            g_ctx.msIdToPointerSlotRva = static_cast<std::uint64_t>(msIdSlotVa - g_ctx.unityPlayer.base);
        }

        return true;
    }

    ResetContext();
    return false;
}

inline bool IsInited()
{
    return g_ctx.process != nullptr && g_ctx.pid != 0 && g_ctx.unityPlayer.base != 0;
}

inline std::uint32_t Pid()
{
    return g_ctx.pid;
}

inline ManagedBackend Runtime()
{
    return g_ctx.runtime;
}

inline std::uintptr_t UnityPlayerBase()
{
    return g_ctx.unityPlayer.base;
}

inline std::uintptr_t GomGlobalSlotVa()
{
    return g_ctx.gomGlobalSlotVa;
}

inline std::uintptr_t MsIdToPointerSlotVa()
{
    return g_ctx.msIdToPointerSlotVa;
}

inline const Offsets& Off()
{
    return g_ctx.off;
}

inline const GomOffsets& GomOff()
{
    return g_ctx.gomOff;
}

inline const CameraOffsets& CamOff()
{
    return g_ctx.camOff;
}

inline const TransformOffsets& TransformOff()
{
    return g_ctx.transformOff;
}

inline WinApiMemoryAccessor Mem()
{
    return WinApiMemoryAccessor(g_ctx.process);
}

inline bool ReadPtr(std::uintptr_t address, std::uintptr_t& out)
{
    if (!IsInited())
    {
        out = 0;
        return false;
    }

    return er2::ReadPtr(Mem(), address, out);
}

inline std::optional<std::uintptr_t> ReadPtr(std::uintptr_t address)
{
    std::uintptr_t out = 0;
    if (!ReadPtr(address, out))
    {
        return std::nullopt;
    }
    return out;
}

template <typename T>
inline bool ReadValue(std::uintptr_t address, T& out)
{
    if (!IsInited())
    {
        out = T{};
        return false;
    }

    return er2::ReadValue(Mem(), address, out);
}

template <typename T>
inline std::optional<T> ReadValue(std::uintptr_t address)
{
    T out{};
    if (!ReadValue(address, out))
    {
        return std::nullopt;
    }
    return out;
}

} // namespace er2
