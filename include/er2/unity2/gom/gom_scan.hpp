#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "gom_scan_validate.hpp"
#include "../metadata/pe.hpp"

#include "../../os/win/win_module.hpp"

namespace er2
{

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

inline std::uintptr_t FindGomGlobalSlotByScanInternal(const IMemoryAccessor& mem, std::uintptr_t moduleBase, std::size_t chunkSize, const GomOffsets& off)
{
    if (!moduleBase)
    {
        return 0;
    }
    if (chunkSize < 0x1000)
    {
        return 0;
    }

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
