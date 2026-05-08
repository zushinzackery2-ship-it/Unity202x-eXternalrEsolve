#pragma once

namespace er2
{

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

} // namespace er2
