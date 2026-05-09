#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../../mem/memory_read.hpp"
#include "../../os/win/win_memory_accessor.hpp"
#include "../../os/win/win_module.hpp"
#include "../metadata/pe.hpp"
#include "gom_scan_validate.hpp"
#include "scan_pattern.hpp"
#include "scan_chain.hpp"
#include "scan_heuristic.hpp"

namespace er2
{
inline std::uintptr_t FindGomGlobalSlotByScanInternal(const IMemoryAccessor& mem, std::uintptr_t moduleBase, std::size_t chunkSize, const GomOffsets& off)
{
    const std::uint64_t tTotalBegin = GomScanNowMs();
    GomScanLog("scan_internal.begin moduleBase=0x%llX chunkSize=0x%zX",
        static_cast<unsigned long long>(moduleBase),
        chunkSize);
    if (!moduleBase)
    {
        GomScanLog("scan_internal.fail reason=module_base_zero");
        return 0;
    }
    if (chunkSize < 0x1000)
    {
        GomScanLog("scan_internal.fail reason=chunk_too_small chunkSize=0x%zX", chunkSize);
        return 0;
    }

    std::uintptr_t gomSlot = 0;
    if (FindGomGlobalSlotByNewPointerChainScan(mem, moduleBase, chunkSize, gomSlot))
    {
        GomScanLog("scan_internal.success method=new_chain gom_slot=0x%llX total_ms=%llu",
            static_cast<unsigned long long>(gomSlot),
            static_cast<unsigned long long>(GomScanNowMs() - tTotalBegin));
        return gomSlot;
    }

    GomScanLog("scan_internal.fallback.begin method=section_heuristic");
    const std::uint64_t tFallbackBegin = GomScanNowMs();
    const std::uintptr_t fallbackSlot = FindGomGlobalSlotBySectionHeuristicInternal(mem, moduleBase, off);
    if (fallbackSlot)
    {
        GomScanLog("scan_internal.success method=section_heuristic gom_slot=0x%llX fallback_ms=%llu total_ms=%llu",
            static_cast<unsigned long long>(fallbackSlot),
            static_cast<unsigned long long>(GomScanNowMs() - tFallbackBegin),
            static_cast<unsigned long long>(GomScanNowMs() - tTotalBegin));
    }
    else
    {
        GomScanLog("scan_internal.fail method=section_heuristic fallback_ms=%llu total_ms=%llu",
            static_cast<unsigned long long>(GomScanNowMs() - tFallbackBegin),
            static_cast<unsigned long long>(GomScanNowMs() - tTotalBegin));
    }
    return fallbackSlot;
}

inline bool FindGomGlobalSlotRvaByScan(const IMemoryAccessor& mem, std::uintptr_t unityPlayerBase, const GomOffsets& off, std::uint64_t& outRva, std::size_t chunkSize = 0x20000)
{
    outRva = 0;
    if (!unityPlayerBase)
    {
        return false;
    }

    const std::uintptr_t slot = FindGomGlobalSlotByScanInternal(mem, unityPlayerBase, chunkSize, off);
    if (!slot)
    {
        return false;
    }

    outRva = static_cast<std::uint64_t>(slot - unityPlayerBase);
    return true;
}

inline bool FindGomGlobalSlotRvaByScan(
    const IMemoryAccessor& mem,
    std::uint32_t pid,
    const wchar_t* moduleName,
    const GomOffsets& off,
    std::uint64_t& outRva,
    std::size_t chunkSize = 0x20000)
{
    outRva = 0;
    if (!moduleName || !moduleName[0])
    {
        return false;
    }

    const std::uintptr_t base = FindModuleBase(pid, moduleName);
    if (!base)
    {
        return false;
    }

    return FindGomGlobalSlotRvaByScan(mem, base, off, outRva, chunkSize);
}

} // namespace er2
