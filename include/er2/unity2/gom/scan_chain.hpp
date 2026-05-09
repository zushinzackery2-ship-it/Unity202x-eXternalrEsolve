#pragma once

namespace er2
{

inline std::uintptr_t FindGomBucketsTableHeadFromSeed(const IMemoryAccessor& mem, std::uintptr_t secondSeedHitAddr)
{
    constexpr std::uintptr_t kStride = 0x18;
    constexpr std::uintptr_t kStep = kStride;
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

inline bool IsHighModuleLikePointer(std::uintptr_t value)
{
    return (value & 0xFFF0000000000000ull) == 0x0000000000000000ull && value >= 0x000007FF00000000ull;
}

inline bool IsPreferredLowGomPointer(std::uintptr_t value)
{
    if (!IsLikelyPtr(value) || IsHighModuleLikePointer(value))
    {
        return false;
    }
    while (value >= 0x100000000ull)
    {
        value >>= 4;
    }
    while (value >= 0x1000000ull)
    {
        value >>= 4;
    }
    while (value >= 0x10000ull)
    {
        value >>= 4;
    }
    return value >= 0x1000ull && value < 0x1100ull;
}

inline std::uintptr_t SelectBestSeedAddr(const IMemoryAccessor& mem, const std::vector<std::uintptr_t>& seedHits, const GomOffsets& off)
{
    std::uintptr_t firstNonGlobal = 0;

    for (std::uintptr_t seedAddr : seedHits)
    {
        std::uintptr_t listHead = 0;
        if (!GetBucketListHead(mem, seedAddr, off, listHead))
        {
            continue;
        }
        if (!IsLikelyPtr(listHead) || IsHighModuleLikePointer(listHead))
        {
            continue;
        }
        if (IsPreferredLowGomPointer(listHead))
        {
            return seedAddr;
        }
        if (!firstNonGlobal)
        {
            firstNonGlobal = seedAddr;
        }
    }

    return firstNonGlobal;
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
    if (!ScanProcessForPatternGlobal(mem, process, seedBytes.data(), seedBytes.size(), seedHits, 64, chunkSize))
    {
        GomScanLog("stage.seed_scan.fail pattern=0x%08X ms=%llu",
            static_cast<unsigned>(kSeedHashMask),
            static_cast<unsigned long long>(GomScanNowMs() - tSeedBegin));
        return false;
    }
    if (seedHits.empty())
    {
        GomScanLog("stage.seed_scan.fail reason=insufficient_hits hits=%zu ms=%llu",
            seedHits.size(),
            static_cast<unsigned long long>(GomScanNowMs() - tSeedBegin));
        return false;
    }
    GomScanLog("stage.seed_scan.ok hits=%zu first_hit=0x%llX ms=%llu",
        seedHits.size(),
        static_cast<unsigned long long>(seedHits[0]),
        static_cast<unsigned long long>(GomScanNowMs() - tSeedBegin));

    const std::uint64_t tSelectBegin = GomScanNowMs();
    const std::uintptr_t bestSeed = SelectBestSeedAddr(mem, seedHits, GomOffsets{});
    if (!bestSeed)
    {
        GomScanLog("stage.seed_select.fail seed_hits=%zu ms=%llu",
            seedHits.size(),
            static_cast<unsigned long long>(GomScanNowMs() - tSelectBegin));
        return false;
    }
    GomScanLog("stage.seed_select.ok seed_addr=0x%llX ms=%llu",
        static_cast<unsigned long long>(bestSeed),
        static_cast<unsigned long long>(GomScanNowMs() - tSelectBegin));

    const std::uintptr_t tableHead = FindGomBucketsTableHeadFromSeed(mem, bestSeed);
    if (!tableHead)
    {
        GomScanLog("stage.table_head.fail seed_addr=0x%llX",
            static_cast<unsigned long long>(bestSeed));
        return false;
    }
    GomScanLog("stage.table_head.ok addr=0x%llX",
        static_cast<unsigned long long>(tableHead));

    const std::uint64_t tScan1Begin = GomScanNowMs();
    const auto tableHeadBytes = ValueBytes(tableHead);
    std::vector<std::uintptr_t> scan1Hits;
    if (!ScanProcessForPatternGlobal(mem, process, tableHeadBytes.data(), tableHeadBytes.size(), scan1Hits, 16, chunkSize))
    {
        GomScanLog("stage.scan1.fail pattern=0x%llX ms=%llu",
            static_cast<unsigned long long>(tableHead),
            static_cast<unsigned long long>(GomScanNowMs() - tScan1Begin));
        return false;
    }
    GomScanLog("stage.scan1.ok hits=%zu ms=%llu",
        scan1Hits.size(),
        static_cast<unsigned long long>(GomScanNowMs() - tScan1Begin));

    const std::uint64_t tScan2Begin = GomScanNowMs();
    std::uintptr_t gomSlot = 0;
    for (std::uintptr_t managerCandidate : scan1Hits)
    {
        const auto managerBytes = ValueBytes(managerCandidate);
        std::vector<std::uintptr_t> scan2Hits;
        if (!ScanModuleSectionsForPointer(mem, moduleBase, managerCandidate, scan2Hits, 4) &&
            !ScanProcessForPatternGlobal(mem, process, managerBytes.data(), managerBytes.size(), scan2Hits, 4, chunkSize))
        {
            continue;
        }
        for (std::uintptr_t hit : scan2Hits)
        {
            std::uintptr_t readBack = 0;
            if (!ReadPtr(mem, hit, readBack) || readBack != managerCandidate)
            {
                continue;
            }
            gomSlot = hit;
            break;
        }
        if (gomSlot)
        {
            break;
        }
    }
    if (!gomSlot)
    {
        GomScanLog("stage.scan2.fail scan1_hits=%zu ms=%llu",
            scan1Hits.size(),
            static_cast<unsigned long long>(GomScanNowMs() - tScan2Begin));
        return false;
    }
    GomScanLog("stage.scan2.ok gom_slot=0x%llX ms=%llu",
        static_cast<unsigned long long>(gomSlot),
        static_cast<unsigned long long>(GomScanNowMs() - tScan2Begin));

    outSlot = gomSlot;
    GomScanLog("new_chain_scan.ok gom_slot=0x%llX total_ms=%llu",
        static_cast<unsigned long long>(gomSlot),
        static_cast<unsigned long long>(GomScanNowMs() - tTotalBegin));
    return true;
}

} // namespace er2
