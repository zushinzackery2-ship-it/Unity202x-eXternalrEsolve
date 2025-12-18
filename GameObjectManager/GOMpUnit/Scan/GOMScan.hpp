#pragma once

#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cctype>

#include "../../../Metadata/PE/Parser.hpp"

#include "../../../MemoryAccessor/Resolver/WinAPIResolver.hpp"
#include "GOMScanValidate.hpp"

namespace UnityExternal
{

inline bool IsSectionWantedForGOMScan(const char* name);

namespace detail_gom_scan
{

inline std::uintptr_t FindGOMGlobalSlotByScanInternal(const IMemoryAccessor& mem,
                                                      std::uintptr_t moduleBase,
                                                      std::size_t chunkSize)
{
    std::uintptr_t bestGomGlobal = 0;
    int bestScore = 0;

    std::vector<detail_metadata::ModuleSection> sections;
    std::uint32_t sizeOfImage = 0;
    if (!detail_metadata::ReadModuleSections(mem, moduleBase, sizeOfImage, sections))
    {
        return 0;
    }

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
    GOMScanTraceCounters& tc = GetGOMScanTraceCounters();
    tc.sizeOfImage = sizeOfImage;
    tc.sectionsTotal = (std::uint32_t)sections.size();
#endif

    std::vector<detail_metadata::ModuleSection> scanSections;
    scanSections.reserve(sections.size());

    for (const auto& s : sections)
    {
        if (!IsSectionWantedForGOMScan(s.name)) continue;

        detail_metadata::ModuleSection ms = s;
        if (ms.rva >= sizeOfImage) continue;
        std::uint32_t maxSize = sizeOfImage - ms.rva;
        if (ms.size == 0 || ms.size > maxSize) ms.size = maxSize;
        if (ms.size == 0) continue;
        scanSections.push_back(ms);
    }

    if (scanSections.empty())
    {
        return 0;
    }

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
    tc.scanSections = (std::uint32_t)scanSections.size();
#endif

    if (chunkSize < 0x1000) chunkSize = 0x1000;
    std::vector<std::uint8_t> chunkBuf;
    chunkBuf.resize(chunkSize);

    for (const auto& sec : scanSections)
    {
        std::uintptr_t secStart = moduleBase + (std::uintptr_t)sec.rva;
        std::uint32_t remaining = sec.size;
        std::uint32_t offset = 0;

        while (remaining > 0)
        {
            std::size_t toRead = remaining > (std::uint32_t)chunkBuf.size() ? chunkBuf.size() : remaining;
            std::uintptr_t chunkAddr = secStart + offset;

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
            ++tc.chunkReads;
#endif
            if (!mem.Read(chunkAddr, chunkBuf.data(), toRead))
            {
#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
                ++tc.chunkReadFails;
#endif
                offset += (std::uint32_t)toRead;
                remaining -= (std::uint32_t)toRead;
                continue;
            }

            std::size_t limit = toRead >= 8 ? (toRead - 8) : 0;

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
            if (toRead >= 8)
            {
                tc.slotTotal += (std::uint64_t)(limit / 8 + 1);
            }
#endif
            for (std::size_t off = 0; off <= limit; off += 8)
            {
                std::uint64_t ptr = 0;
                std::memcpy(&ptr, chunkBuf.data() + off, sizeof(ptr));

                if (!IsLikelyPtr((std::uintptr_t)ptr)) continue;

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
                ++tc.likelyPtr;
#endif

                std::uintptr_t manager = (std::uintptr_t)ptr;
                ManagerCandidateCheck r = CheckGameObjectManagerCandidateBlindScan(mem, manager);
                if (!r.ok) continue;

                if (r.score > bestScore)
                {
                    bestScore = r.score;
                    bestGomGlobal = chunkAddr + (std::uintptr_t)off;

#if defined(UNITYEXTERNAL_GOMSCAN_TRACE)
                    tc.bestGomGlobal = bestGomGlobal;
                    tc.bestManager = manager;
                    tc.bestScore = bestScore;
#endif
                }
            }

            offset += (std::uint32_t)toRead;
            remaining -= (std::uint32_t)toRead;
        }
    }

    return bestGomGlobal;
}

} // namespace detail_gom_scan

inline bool IsSectionWantedForGOMScan(const char* name)
{
    if (!name || !name[0]) return false;

    std::string s(name);
    for (auto& c : s)
    {
        c = (char)std::tolower((unsigned char)c);
    }

    return s == ".data" || s == ".rdata";
}
inline std::uint64_t FindGOMGlobalByScan()
{
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return 0;

    const std::uint32_t pid32 = GetTargetPid();
    if (!pid32) return 0;

    const DWORD pid = static_cast<DWORD>(pid32);
    const std::uintptr_t unityPlayerBase = FindModuleBase(pid, L"UnityPlayer.dll");
    if (!unityPlayerBase) return 0;

    const std::uintptr_t gomGlobal = detail_gom_scan::FindGOMGlobalSlotByScanInternal(*acc, unityPlayerBase, 0x20000);
    if (!gomGlobal) return 0;

    return static_cast<std::uint64_t>(gomGlobal - unityPlayerBase);
}

}
