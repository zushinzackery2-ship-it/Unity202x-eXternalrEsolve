#pragma once

#include <Windows.h>
#include <algorithm>
#include <chrono>

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
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

inline std::uint64_t AutoInitNowMs()
{
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count());
}

inline void AutoInitLog(const char* fmt, ...)
{
    std::fprintf(stderr, "[AUTO_INIT] ");
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
}

struct Context
{
    std::uint32_t pid = 0;
    HANDLE process = nullptr;

    ManagedBackend runtime = ManagedBackend::Mono;

    ModuleInfo unityPlayer;
    UnityPlayerRange unityPlayerRange;
    ModuleInfo gameAssembly;

    Offsets off;
    GomOffsets gomOff;
    CameraOffsets camOff;
    TransformOffsets transformOff;

    std::uintptr_t gomGlobalSlotVa = 0;
    std::uint64_t gomGlobalSlotRva = 0;

    std::uintptr_t msIdToPointerSlotVa = 0;
    std::uint64_t msIdToPointerSlotRva = 0;

    std::uintptr_t typeInfoTable = 0;
    std::uint32_t typeInfoCount = 0;
    std::unordered_map<std::uint32_t, std::string> byvalToFullName;
    std::unordered_map<std::string, std::uint32_t> fullNameToByval;
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
    const std::uint64_t tTotalBegin = AutoInitNowMs();
    std::vector<std::uint32_t> pids = FindUnityWndClassPids();
    AutoInitLog("find_unity_wndclass_pids count=%zu", pids.size());
    if (pids.empty())
    {
        pids = FindPidsWithModule(L"UnityPlayer.dll");
        AutoInitLog("fallback_find_pids_with_module(UnityPlayer.dll) count=%zu", pids.size());
    }

    if (pids.empty())
    {
        AutoInitLog("fail reason=no_candidate_process");
        return false;
    }

    std::sort(pids.begin(), pids.end());
    pids.erase(std::unique(pids.begin(), pids.end()), pids.end());
    AutoInitLog("candidate_pid_count=%zu", pids.size());

    for (const std::uint32_t pid : pids)
    {
        const std::uint64_t tPidBegin = AutoInitNowMs();
        AutoInitLog("try_pid pid=%u", pid);

        ModuleInfo up;
        if (!GetRemoteModuleInfo(pid, L"UnityPlayer.dll", up))
        {
            AutoInitLog("skip_pid pid=%u reason=no_unityplayer_module", pid);
            continue;
        }

        ModuleInfo ga;
        const bool isIl2Cpp = GetRemoteModuleInfo(pid, L"GameAssembly.dll", ga);
        const ManagedBackend runtime = isIl2Cpp ? ManagedBackend::Il2Cpp : ManagedBackend::Mono;
        AutoInitLog("pid=%u runtime_candidate=%s", pid, isIl2Cpp ? "IL2CPP" : "Mono");

        if (!InitSettings(pid, runtime))
        {
            AutoInitLog("skip_pid pid=%u reason=init_settings_failed", pid);
            continue;
        }

        if (isIl2Cpp)
        {
            g_ctx.gameAssembly = ga;
        }

        WinApiMemoryAccessor mem(g_ctx.process);
        const std::uint64_t tGomBegin = AutoInitNowMs();
        std::uint64_t gomSlotRva = 0;
        if (!FindGomGlobalSlotRvaByScan(mem, g_ctx.unityPlayer.base, g_ctx.gomOff, gomSlotRva))
        {
            AutoInitLog("pid=%u gom_scan_failed ms=%llu",
                pid,
                static_cast<unsigned long long>(AutoInitNowMs() - tGomBegin));
            ResetContext();
            continue;
        }
        AutoInitLog("pid=%u gom_scan_ok gom_rva=0x%llX ms=%llu",
            pid,
            static_cast<unsigned long long>(gomSlotRva),
            static_cast<unsigned long long>(AutoInitNowMs() - tGomBegin));

        g_ctx.gomGlobalSlotRva = gomSlotRva;
        g_ctx.gomGlobalSlotVa = g_ctx.unityPlayer.base + static_cast<std::uintptr_t>(gomSlotRva);

        // MSID 扫描可选，失败不影响 GOM 功能
        std::uintptr_t msIdSlotVa = 0;
        const std::uint64_t tMsidBegin = AutoInitNowMs();
        if (FindMsIdToPointerSlotVaByScan(mem, g_ctx.unityPlayer, g_ctx.gomOff, msIdSlotVa, nullptr, g_ctx.gomGlobalSlotVa))
        {
            g_ctx.msIdToPointerSlotVa = msIdSlotVa;
            g_ctx.msIdToPointerSlotRva = static_cast<std::uint64_t>(msIdSlotVa - g_ctx.unityPlayer.base);
            AutoInitLog("pid=%u msid_scan_ok slot_va=0x%llX ms=%llu",
                pid,
                static_cast<unsigned long long>(msIdSlotVa),
                static_cast<unsigned long long>(AutoInitNowMs() - tMsidBegin));
        }
        else
        {
            AutoInitLog("pid=%u msid_scan_failed ms=%llu",
                pid,
                static_cast<unsigned long long>(AutoInitNowMs() - tMsidBegin));
        }

        AutoInitLog("success pid=%u total_pid_ms=%llu total_ms=%llu",
            pid,
            static_cast<unsigned long long>(AutoInitNowMs() - tPidBegin),
            static_cast<unsigned long long>(AutoInitNowMs() - tTotalBegin));
        return true;
    }

    AutoInitLog("fail reason=all_candidates_failed total_ms=%llu",
        static_cast<unsigned long long>(AutoInitNowMs() - tTotalBegin));
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
