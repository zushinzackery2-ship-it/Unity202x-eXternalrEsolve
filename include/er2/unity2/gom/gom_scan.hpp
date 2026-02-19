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

namespace er2
{

inline std::uint64_t GomScanNowMs()
{
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count());
}

inline void GomScanLog(const char* fmt, ...)
{
    std::fprintf(stderr, "[GOM_SCAN] ");
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
}

inline bool IsSectionWantedForGomScan(const char* name)
{
    if (!name || !name[0])
    {
        return false;
    }

    std::string s(name);
    for (auto& c : s)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return s == ".data" || s == ".rdata";
}

inline bool IsReadableProtectForGlobalScan(std::uint32_t protect)
{
    const std::uint32_t p = protect & 0xFFu;
    return p == PAGE_READONLY ||
        p == PAGE_READWRITE ||
        p == PAGE_WRITECOPY ||
        p == PAGE_EXECUTE_READ ||
        p == PAGE_EXECUTE_READWRITE ||
        p == PAGE_EXECUTE_WRITECOPY;
}

inline bool ScanProcessForPatternGlobal(
    const IMemoryAccessor& mem,
    HANDLE process,
    const std::uint8_t* pattern,
    std::size_t patternSize,
    std::vector<std::uintptr_t>& outHits,
    std::size_t maxHits,
    std::size_t chunkSize)
{
    if (!process || !pattern || patternSize == 0)
    {
        return false;
    }
    if (chunkSize < 0x1000)
    {
        chunkSize = 0x1000;
    }
    if (chunkSize < patternSize)
    {
        chunkSize = patternSize;
    }

    outHits.clear();

    constexpr std::uintptr_t kUserMax = 0x00007FFFFFFFFFFFull;
    MEMORY_BASIC_INFORMATION mbi{};
    std::uintptr_t cursor = 0;

    std::vector<std::uint8_t> chunk;
    std::vector<std::uint8_t> tail;

    while (cursor < kUserMax)
    {
        const SIZE_T q = VirtualQueryEx(process, reinterpret_cast<LPCVOID>(cursor), &mbi, sizeof(mbi));
        if (q == 0)
        {
            break;
        }

        const std::uintptr_t regionBase = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
        const std::uintptr_t regionSize = static_cast<std::uintptr_t>(mbi.RegionSize);
        const std::uintptr_t regionEnd = regionBase + regionSize;
        if (regionEnd <= cursor)
        {
            break;
        }
        cursor = regionEnd;

        if (regionSize < patternSize)
        {
            continue;
        }
        if (mbi.State != MEM_COMMIT)
        {
            continue;
        }
        if ((mbi.Protect & PAGE_GUARD) != 0 || (mbi.Protect & PAGE_NOACCESS) != 0)
        {
            continue;
        }
        if (!IsReadableProtectForGlobalScan(static_cast<std::uint32_t>(mbi.Protect)))
        {
            continue;
        }

        std::uintptr_t readAddr = regionBase;
        tail.clear();
        while (readAddr < regionEnd)
        {
            const std::uintptr_t remain = regionEnd - readAddr;
            const std::size_t toRead = static_cast<std::size_t>(remain > chunkSize ? chunkSize : remain);

            chunk.resize(tail.size() + toRead);
            if (!tail.empty())
            {
                std::memcpy(chunk.data(), tail.data(), tail.size());
            }

            if (!mem.Read(readAddr, chunk.data() + tail.size(), toRead))
            {
                tail.clear();
                readAddr += static_cast<std::uintptr_t>(toRead);
                continue;
            }

            const std::size_t total = tail.size() + toRead;
            if (total >= patternSize)
            {
                const std::uintptr_t blockBase = readAddr - static_cast<std::uintptr_t>(tail.size());
                const std::size_t last = total - patternSize;
                for (std::size_t i = 0; i <= last; ++i)
                {
                    if (std::memcmp(chunk.data() + i, pattern, patternSize) == 0)
                    {
                        outHits.push_back(blockBase + static_cast<std::uintptr_t>(i));
                        if (maxHits != 0 && outHits.size() >= maxHits)
                        {
                            return true;
                        }
                    }
                }
            }

            std::size_t keep = 0;
            if (patternSize > 1)
            {
                const std::size_t maxKeep = patternSize - 1;
                keep = (total > maxKeep) ? maxKeep : total;
            }
            tail.clear();
            if (keep > 0)
            {
                tail.resize(keep);
                std::memcpy(tail.data(), chunk.data() + (total - keep), keep);
            }

            readAddr += static_cast<std::uintptr_t>(toRead);
        }
    }

    return !outHits.empty();
}

template <typename T>
inline std::array<std::uint8_t, sizeof(T)> ValueBytes(T value)
{
    std::array<std::uint8_t, sizeof(T)> out{};
    std::memcpy(out.data(), &value, sizeof(T));
    return out;
}

inline std::uintptr_t FindGomBucketsTableHeadFromSeed(const IMemoryAccessor& mem, std::uintptr_t secondSeedHitAddr)
{
    constexpr std::uintptr_t kStride = 0x18;
    constexpr std::uintptr_t kStep = 0x8;
    constexpr std::size_t kMaxUpSearchSteps = 0x4000;
    constexpr std::uint32_t kHeadHashMask = 0xFFFFFFFFu;
    constexpr std::uint32_t kPreHeadHashMask = 0x00000000u;

    for (std::size_t i = 0; i <= kMaxUpSearchSteps; ++i)
    {
        const std::uintptr_t back = static_cast<std::uintptr_t>(i) * kStep;
        if (secondSeedHitAddr < back)
        {
            break;
        }

        const std::uintptr_t cur = secondSeedHitAddr - back;
        std::uint32_t curHashMask = 0;
        if (!ReadValue(mem, cur, curHashMask))
        {
            continue;
        }
        if (curHashMask != kHeadHashMask)
        {
            continue;
        }
        if (cur < kStride)
        {
            continue;
        }

        std::uint32_t prevHashMask = 0;
        if (!ReadValue(mem, cur - kStride, prevHashMask))
        {
            continue;
        }

        if (prevHashMask == kPreHeadHashMask)
        {
            return cur;
        }
    }

    return 0;
}

inline bool FindGomGlobalSlotByNewPointerChainScan(const IMemoryAccessor& mem, std::uintptr_t moduleBase, std::size_t chunkSize, std::uintptr_t& outSlot)
{
    outSlot = 0;
    const std::uint64_t tTotalBegin = GomScanNowMs();
    GomScanLog("new_chain_scan.begin moduleBase=0x%llX chunkSize=0x%zX",
        static_cast<unsigned long long>(moduleBase),
        chunkSize);

    const auto* winMem = dynamic_cast<const WinApiMemoryAccessor*>(&mem);
    if (!winMem)
    {
        GomScanLog("new_chain_scan.fail reason=memory_accessor_not_winapi");
        return false;
    }

    const HANDLE process = winMem->GetProcessHandle();
    if (!process)
    {
        GomScanLog("new_chain_scan.fail reason=invalid_process_handle");
        return false;
    }

    constexpr std::uint32_t kSeedHashMask = 0x01F266ECu;
    const std::uint64_t tSeedBegin = GomScanNowMs();
    const auto seedBytes = ValueBytes(kSeedHashMask);
    std::vector<std::uintptr_t> seedHits;
    if (!ScanProcessForPatternGlobal(mem, process, seedBytes.data(), seedBytes.size(), seedHits, 2, chunkSize))
    {
        GomScanLog("stage.seed_scan.fail pattern=0x%08X ms=%llu",
            static_cast<unsigned>(kSeedHashMask),
            static_cast<unsigned long long>(GomScanNowMs() - tSeedBegin));
        return false;
    }
    if (seedHits.size() < 2)
    {
        GomScanLog("stage.seed_scan.fail reason=insufficient_hits hits=%zu ms=%llu",
            seedHits.size(),
            static_cast<unsigned long long>(GomScanNowMs() - tSeedBegin));
        return false;
    }
    GomScanLog("stage.seed_scan.ok hits=%zu second_hit=0x%llX ms=%llu",
        seedHits.size(),
        static_cast<unsigned long long>(seedHits[1]),
        static_cast<unsigned long long>(GomScanNowMs() - tSeedBegin));

    const std::uintptr_t secondSeedHitAddr = seedHits[1];
    const std::uint64_t tHeadBegin = GomScanNowMs();
    const std::uintptr_t tableHeadAddr = FindGomBucketsTableHeadFromSeed(mem, secondSeedHitAddr);
    if (!tableHeadAddr)
    {
        GomScanLog("stage.table_head.fail second_hit=0x%llX ms=%llu",
            static_cast<unsigned long long>(secondSeedHitAddr),
            static_cast<unsigned long long>(GomScanNowMs() - tHeadBegin));
        return false;
    }
    GomScanLog("stage.table_head.ok table_head=0x%llX ms=%llu",
        static_cast<unsigned long long>(tableHeadAddr),
        static_cast<unsigned long long>(GomScanNowMs() - tHeadBegin));

    const std::uint64_t tLevel1Begin = GomScanNowMs();
    const auto headAddrBytes = ValueBytes(tableHeadAddr);
    std::vector<std::uintptr_t> level1Hits;
    if (!ScanProcessForPatternGlobal(mem, process, headAddrBytes.data(), headAddrBytes.size(), level1Hits, 2, chunkSize))
    {
        GomScanLog("stage.level1_scan.fail pattern=0x%llX ms=%llu",
            static_cast<unsigned long long>(tableHeadAddr),
            static_cast<unsigned long long>(GomScanNowMs() - tLevel1Begin));
        return false;
    }
    if (level1Hits.size() != 1)
    {
        GomScanLog("stage.level1_scan.fail reason=non_unique_hits hits=%zu ms=%llu",
            level1Hits.size(),
            static_cast<unsigned long long>(GomScanNowMs() - tLevel1Begin));
        return false;
    }
    GomScanLog("stage.level1_scan.ok level1_addr=0x%llX ms=%llu",
        static_cast<unsigned long long>(level1Hits[0]),
        static_cast<unsigned long long>(GomScanNowMs() - tLevel1Begin));

    const std::uintptr_t level1Addr = level1Hits[0];
    const std::uint64_t tLevel2Begin = GomScanNowMs();
    const auto level1Bytes = ValueBytes(level1Addr);
    std::vector<std::uintptr_t> gomHits;
    if (!ScanProcessForPatternGlobal(mem, process, level1Bytes.data(), level1Bytes.size(), gomHits, 2, chunkSize))
    {
        GomScanLog("stage.level2_scan.fail pattern=0x%llX ms=%llu",
            static_cast<unsigned long long>(level1Addr),
            static_cast<unsigned long long>(GomScanNowMs() - tLevel2Begin));
        return false;
    }
    if (gomHits.size() != 1)
    {
        GomScanLog("stage.level2_scan.fail reason=non_unique_hits hits=%zu ms=%llu",
            gomHits.size(),
            static_cast<unsigned long long>(GomScanNowMs() - tLevel2Begin));
        return false;
    }
    GomScanLog("stage.level2_scan.ok gom_slot=0x%llX ms=%llu",
        static_cast<unsigned long long>(gomHits[0]),
        static_cast<unsigned long long>(GomScanNowMs() - tLevel2Begin));

    const std::uintptr_t gomSlot = gomHits[0];
    if (!IsLikelyPtr(gomSlot))
    {
        GomScanLog("new_chain_scan.fail reason=slot_not_likely_ptr gom_slot=0x%llX",
            static_cast<unsigned long long>(gomSlot));
        return false;
    }

    const std::uint64_t tRangeBegin = GomScanNowMs();
    std::uint32_t sizeOfImage = 0;
    std::vector<ModuleSection> sections;
    if (ReadModuleSections(mem, moduleBase, sizeOfImage, sections))
    {
        const std::uintptr_t moduleEnd = moduleBase + static_cast<std::uintptr_t>(sizeOfImage);
        if (gomSlot < moduleBase || gomSlot >= moduleEnd)
        {
            GomScanLog("stage.module_range.fail gom_slot=0x%llX module=[0x%llX,0x%llX) ms=%llu",
                static_cast<unsigned long long>(gomSlot),
                static_cast<unsigned long long>(moduleBase),
                static_cast<unsigned long long>(moduleEnd),
                static_cast<unsigned long long>(GomScanNowMs() - tRangeBegin));
            return false;
        }
    }
    GomScanLog("stage.module_range.ok ms=%llu",
        static_cast<unsigned long long>(GomScanNowMs() - tRangeBegin));

    outSlot = gomSlot;
    GomScanLog("new_chain_scan.ok gom_slot=0x%llX total_ms=%llu",
        static_cast<unsigned long long>(gomSlot),
        static_cast<unsigned long long>(GomScanNowMs() - tTotalBegin));
    return true;
}

inline std::uintptr_t FindGomGlobalSlotBySectionHeuristicInternal(const IMemoryAccessor& mem, std::uintptr_t moduleBase, const GomOffsets& off)
{
    std::uintptr_t bestGomGlobal = 0;
    int bestScore = 0;

    std::vector<ModuleSection> sections;
    std::uint32_t sizeOfImage = 0;
    if (!ReadModuleSections(mem, moduleBase, sizeOfImage, sections))
    {
        return 0;
    }

    const std::uintptr_t moduleEnd = moduleBase + static_cast<std::uintptr_t>(sizeOfImage);
    const int maxScore = 20 + 80 + 64 + 64 + 20;

    std::vector<ModuleSection> scanSections;
    scanSections.reserve(sections.size());

    for (const auto& s : sections)
    {
        if (!IsSectionWantedForGomScan(s.name.c_str()))
        {
            continue;
        }

        ModuleSection ms = s;
        if (ms.rva >= sizeOfImage)
        {
            continue;
        }

        const std::uint32_t maxSize = sizeOfImage - ms.rva;
        if (ms.size == 0 || ms.size > maxSize)
        {
            ms.size = maxSize;
        }
        if (ms.size == 0)
        {
            continue;
        }
        scanSections.push_back(ms);
    }

    if (scanSections.empty())
    {
        return 0;
    }

    std::vector<std::uint8_t> secBuf;

    for (const auto& sec : scanSections)
    {
        const std::uintptr_t secStart = moduleBase + static_cast<std::uintptr_t>(sec.rva);

        std::string secNameLower = sec.name;
        for (auto& c : secNameLower)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const bool isRdata = (secNameLower == ".rdata");

        secBuf.clear();
        secBuf.resize(sec.size);
        if (!mem.Read(secStart, secBuf.data(), secBuf.size()))
        {
            continue;
        }

        const std::size_t limit = secBuf.size() >= 8 ? (secBuf.size() - 8) : 0;
        for (std::size_t off8 = 0; off8 <= limit; off8 += 8)
        {
            std::uint64_t ptr = 0;
            std::memcpy(&ptr, secBuf.data() + off8, sizeof(ptr));

            if (!IsLikelyPtr(static_cast<std::uintptr_t>(ptr)))
            {
                continue;
            }

            const std::uintptr_t manager = static_cast<std::uintptr_t>(ptr);

            if (isRdata)
            {
                if (manager >= moduleBase && manager < moduleEnd)
                {
                    continue;
                }
            }

            const ManagerCandidateCheck r = CheckGameObjectManagerCandidateBlindScan(mem, manager, off);
            if (!r.ok)
            {
                continue;
            }

            if (r.score > bestScore)
            {
                bestScore = r.score;
                bestGomGlobal = secStart + static_cast<std::uintptr_t>(off8);

                if (bestScore >= maxScore)
                {
                    return bestGomGlobal;
                }
            }
        }
    }

    return bestGomGlobal;
}

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
