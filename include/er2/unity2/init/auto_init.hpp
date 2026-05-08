#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <vector>

#include "context_api.hpp"
#include "../../os/win/win_window.hpp"
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
        if (!GetContextModuleInfo(pid, L"UnityPlayer.dll", up))
        {
            AutoInitLog("skip_pid pid=%u reason=no_unityplayer_module", pid);
            continue;
        }

        ModuleInfo ga;
        const bool isIl2Cpp = GetContextModuleInfo(pid, L"GameAssembly.dll", ga);
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

        const IMemoryAccessor& mem = Mem();
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

} // namespace er2
