#pragma once

namespace er2
{

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

} // namespace er2
