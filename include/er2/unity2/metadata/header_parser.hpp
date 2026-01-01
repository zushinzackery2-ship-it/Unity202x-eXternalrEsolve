#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "../../mem/memory_read.hpp"

namespace er2
{

struct MetadataScoreResult
{
    int score;
    std::uint32_t maxEnd;
};

inline std::uint32_t ReadU32LE(const std::uint8_t* p)
{
    return (std::uint32_t)p[0] |
           ((std::uint32_t)p[1] << 8) |
           ((std::uint32_t)p[2] << 16) |
           ((std::uint32_t)p[3] << 24);
}

static constexpr std::uint32_t kMetadataHeaderPairs[][2] = {
    {0x08, 0x0C}, {0x10, 0x14}, {0x18, 0x1C}, {0x20, 0x24}, {0x28, 0x2C}, {0x30, 0x34}, {0x38, 0x3C},
    {0x40, 0x44}, {0x48, 0x4C}, {0x50, 0x54}, {0x58, 0x5C}, {0x60, 0x64}, {0x68, 0x6C}, {0x70, 0x74},
    {0x78, 0x7C}, {0x80, 0x84}, {0x88, 0x8C}, {0x90, 0x94}, {0x98, 0x9C}, {0xA0, 0xA4}, {0xA8, 0xAC},
    {0xB0, 0xB4}, {0xB8, 0xBC}, {0xC0, 0xC4}, {0xC8, 0xCC}, {0xD0, 0xD4}, {0xD8, 0xDC}, {0xE0, 0xE4},
    {0xE8, 0xEC}, {0xF0, 0xF4}, {0xF8, 0xFC},
};

inline MetadataScoreResult ScoreMetadataHeader(
    const std::uint8_t* buf,
    std::size_t bufSize,
    std::size_t off,
    bool strictVersion,
    std::uint32_t requiredVersion)
{
    if (!buf)
    {
        return {0, 0};
    }
    if ((off & 3u) != 0u)
    {
        return {0, 0};
    }
    if (off + 0x120u > bufSize)
    {
        return {0, 0};
    }

    const std::uint32_t version = ReadU32LE(buf + off + 0x04u);
    if (requiredVersion != 0 && version != requiredVersion)
    {
        return {0, 0};
    }
    if (requiredVersion == 0 && strictVersion)
    {
        if (version < 10 || version > 100)
        {
            return {0, 0};
        }
    }

    const std::uint32_t stringOffset = ReadU32LE(buf + off + 0x08u);
    const std::uint32_t stringSize = ReadU32LE(buf + off + 0x0Cu);
    if (stringOffset == 0 || stringSize == 0)
    {
        return {0, 0};
    }
    if ((stringOffset & 3u) != 0u)
    {
        return {0, 0};
    }
    if (stringOffset < 0x100u)
    {
        return {0, 0};
    }

    const std::uint32_t imagesOffset = ReadU32LE(buf + off + 0xA8u);
    const std::uint32_t imagesSize = ReadU32LE(buf + off + 0xACu);
    const std::uint32_t assembliesOffset = ReadU32LE(buf + off + 0xB0u);
    const std::uint32_t assembliesSize = ReadU32LE(buf + off + 0xB4u);
    if (imagesSize == 0 || assembliesSize == 0)
    {
        return {0, 0};
    }
    if (imagesOffset == 0 || assembliesOffset == 0)
    {
        return {0, 0};
    }
    if ((imagesOffset & 3u) != 0u || (assembliesOffset & 3u) != 0u)
    {
        return {0, 0};
    }
    if (imagesOffset < 0x100u || assembliesOffset < 0x100u)
    {
        return {0, 0};
    }
    if ((imagesSize % 0x28u) != 0u)
    {
        return {0, 0};
    }
    if ((assembliesSize % 0x40u) != 0u)
    {
        return {0, 0};
    }

    const std::uint32_t imagesCount = imagesSize / 0x28u;
    const std::uint32_t assembliesCount = assembliesSize / 0x40u;
    if (imagesCount == 0 || imagesCount > 200000)
    {
        return {0, 0};
    }
    if (assembliesCount == 0 || assembliesCount > 200000)
    {
        return {0, 0};
    }

    std::uint32_t maxEnd = 0;
    int nonzero = 0;
    for (const auto& pr : kMetadataHeaderPairs)
    {
        const std::uint32_t o = ReadU32LE(buf + off + pr[0]);
        const std::uint32_t s = ReadU32LE(buf + off + pr[1]);
        if (s == 0)
        {
            continue;
        }
        if (o == 0)
        {
            return {0, 0};
        }
        if ((o & 3u) != 0u)
        {
            return {0, 0};
        }
        if (o < 0x100u)
        {
            return {0, 0};
        }

        ++nonzero;
        const std::uint32_t end = o + s;
        if (end > maxEnd)
        {
            maxEnd = end;
        }
    }

    if (nonzero < 6)
    {
        return {0, 0};
    }
    if (maxEnd < 0x20000u || maxEnd > 0x40000000u)
    {
        return {0, 0};
    }
    if (stringOffset + stringSize > maxEnd)
    {
        return {0, 0};
    }
    if (imagesOffset + imagesSize > maxEnd)
    {
        return {0, 0};
    }
    if (assembliesOffset + assembliesSize > maxEnd)
    {
        return {0, 0};
    }

    long long score = 0;
    score += (long long)nonzero * 100000LL;
    score += (long long)(maxEnd / 0x1000u);
    score += (long long)(std::min<std::uint32_t>(stringSize, 0x40000000u) / 0x1000u);
    score += (long long)std::min<std::uint32_t>(imagesCount, 200000u);
    score += (long long)std::min<std::uint32_t>(assembliesCount, 200000u);

    return {(int)score, maxEnd};
}

inline bool CalcTotalSizeFromHeader(const IMemoryAccessor& mem, std::uintptr_t metaBase, std::uint32_t& outTotalSize)
{
    outTotalSize = 0;

    std::uint32_t maxEnd = 0;
    for (const auto& pr : kMetadataHeaderPairs)
    {
        std::uint32_t o = 0;
        std::uint32_t s = 0;
        if (!ReadValue(mem, metaBase + pr[0], o))
        {
            return false;
        }
        if (!ReadValue(mem, metaBase + pr[1], s))
        {
            return false;
        }
        if (s == 0)
        {
            continue;
        }

        const std::uint32_t end = o + s;
        if (end > maxEnd)
        {
            maxEnd = end;
        }
    }

    if (maxEnd == 0)
    {
        return false;
    }

    outTotalSize = maxEnd;
    return true;
}

} // namespace er2
