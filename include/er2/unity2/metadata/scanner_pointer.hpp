#pragma once

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "header_parser.hpp"
#include "pe.hpp"

namespace er2
{

struct FoundMetadata
{
    std::uintptr_t ptrAddr = 0;
    std::uintptr_t metaBase = 0;
    std::uint32_t maxEnd = 0;
    int score = 0;
};

inline bool IsSectionWanted(const char* name)
{
    if (!name || !name[0])
    {
        return false;
    }

    std::string s(name);
    for (auto& c : s)
    {
        c = (char)std::tolower((unsigned char)c);
    }

    return s == ".data" || s == ".rdata" || s == ".pdata" || s == ".tls" || s == ".reloc";
}

inline FoundMetadata FindMetadataPointerByScore(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::size_t chunkSize,
    std::size_t maxPages,
    double maxSeconds,
    bool strictVersion,
    std::uint32_t requiredVersion)
{
    FoundMetadata best;

    auto start = std::chrono::steady_clock::now();

    std::vector<ModuleSection> sections;
    std::uint32_t sizeOfImage = 0;
    if (!ReadModuleSections(mem, moduleBase, sizeOfImage, sections))
    {
        return best;
    }

    std::vector<ModuleSection> scanSections;
    scanSections.reserve(sections.size());
    for (const auto& s : sections)
    {
        if (!IsSectionWanted(s.name.c_str()))
        {
            continue;
        }

        ModuleSection ms = s;
        if (ms.rva >= sizeOfImage)
        {
            continue;
        }

        std::uint32_t maxSize = sizeOfImage - ms.rva;
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
        return best;
    }

    std::unordered_map<std::uintptr_t, std::vector<std::pair<std::uintptr_t, std::uintptr_t>>> pageMap;
    pageMap.reserve(maxPages);

    auto validatePages = [&]()
    {
        if (pageMap.empty())
        {
            return;
        }

        std::vector<std::uint8_t> pageBuf;
        pageBuf.resize(0x1000);

        for (const auto& it : pageMap)
        {
            const std::uintptr_t page = it.first;
            if (!mem.Read(page, pageBuf.data(), pageBuf.size()))
            {
                continue;
            }

            for (const auto& cand : it.second)
            {
                const std::uintptr_t ptrAddr = cand.first;
                const std::uintptr_t ptr = cand.second;
                const std::uintptr_t delta = ptr - page;
                if ((std::size_t)delta + 0x120u > pageBuf.size())
                {
                    continue;
                }

                const MetadataScoreResult sr = ScoreMetadataHeader(pageBuf.data(), pageBuf.size(), (std::size_t)delta, strictVersion, requiredVersion);
                if (sr.score <= 0)
                {
                    continue;
                }

                if (sr.score > best.score)
                {
                    best.score = sr.score;
                    best.maxEnd = sr.maxEnd;
                    best.ptrAddr = ptrAddr;
                    best.metaBase = ptr;
                }
            }

            if (best.metaBase)
            {
                std::uint8_t tail = 0;
                const std::uintptr_t endMinus1 = best.metaBase + (std::uintptr_t)best.maxEnd - 1;
                if (mem.Read(endMinus1, &tail, 1))
                {
                    return;
                }
            }
        }
    };

    if (chunkSize < 0x1000)
    {
        return best;
    }

    std::vector<std::uint8_t> chunkBuf;
    chunkBuf.resize(chunkSize);

    for (const auto& sec : scanSections)
    {
        const std::uintptr_t secStart = moduleBase + (std::uintptr_t)sec.rva;
        std::uint32_t remaining = sec.size;
        std::uint32_t offset = 0;

        while (remaining > 0)
        {
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (maxSeconds > 0.0 && elapsed > maxSeconds)
            {
                break;
            }

            const std::size_t toRead = remaining > (std::uint32_t)chunkBuf.size() ? chunkBuf.size() : remaining;
            const std::uintptr_t chunkAddr = secStart + offset;
            if (!mem.Read(chunkAddr, chunkBuf.data(), toRead))
            {
                offset += (std::uint32_t)toRead;
                remaining -= (std::uint32_t)toRead;
                continue;
            }

            const std::size_t limit = toRead >= 8 ? (toRead - 8) : 0;
            for (std::size_t off = 0; off <= limit; off += 8)
            {
                std::uint64_t ptr = 0;
                std::memcpy(&ptr, chunkBuf.data() + off, sizeof(ptr));
                if (ptr == 0)
                {
                    continue;
                }
                if (ptr < 0x0000000000010000ull || ptr > 0x00007FFFFFFFFFFFull)
                {
                    continue;
                }
                if ((ptr & 7ull) != 0ull)
                {
                    continue;
                }

                const std::uintptr_t page = (std::uintptr_t)ptr & ~0xFFFull;
                const std::uintptr_t ptrAddr = chunkAddr + (std::uintptr_t)off;
                pageMap[page].push_back({ptrAddr, (std::uintptr_t)ptr});

                if (pageMap.size() >= maxPages)
                {
                    validatePages();
                    pageMap.clear();
                    if (best.metaBase)
                    {
                        return best;
                    }
                }
            }

            validatePages();
            pageMap.clear();
            if (best.metaBase)
            {
                return best;
            }

            offset += (std::uint32_t)toRead;
            remaining -= (std::uint32_t)toRead;
        }
    }

    if (!best.metaBase)
    {
        validatePages();
        pageMap.clear();
    }

    return best;
}

} // namespace er2
