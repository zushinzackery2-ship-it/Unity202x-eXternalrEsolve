#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <cstdint>
#include <vector>

#include "../../Core/UnityExternalMemoryConfig.hpp"
#include "../Core/Config.hpp"
#include "../PE/Parser.hpp"
#include "../Header/Parser.hpp"
#include "../Scanner/Pointer.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

inline std::vector<std::uint8_t> ReadMetadataRegion(const IMemoryAccessor& mem, std::uintptr_t base, std::uint32_t size)
{
    std::vector<std::uint8_t> result;
    if (!base || size == 0) return result;

    constexpr std::size_t kChunk = 1024 * 1024;
    result.reserve(size);

    std::vector<std::uint8_t> buf;
    buf.resize(kChunk);

    std::uint32_t remaining = size;
    std::uint32_t offset = 0;
    while (remaining > 0)
    {
        std::size_t toRead = remaining > (std::uint32_t)kChunk ? kChunk : remaining;
        if (!ReadRemote(mem, base + offset, buf.data(), toRead))
        {
            result.clear();
            return result;
        }
        result.insert(result.end(), buf.begin(), buf.begin() + toRead);
        offset += (std::uint32_t)toRead;
        remaining -= (std::uint32_t)toRead;
    }

    return result;
}

inline std::vector<std::uint8_t> ExportMetadataImpl(std::uint32_t requiredVersion, bool strictVersion)
{
    std::vector<std::uint8_t> result;

    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return result;

    std::uintptr_t moduleBase = GetMetadataTargetModule();
    if (!moduleBase) return result;

    std::uint32_t sizeOfImage = 0;
    std::vector<ModuleSection> sections;
    if (!ReadModuleSections(*acc, moduleBase, sizeOfImage, sections))
    {
        sizeOfImage = 0x2000000u;
    }

    FoundMeta found = FindMetadataPointerByScore(
        *acc,
        moduleBase,
        sizeOfImage,
        0x200000u,
        8192,
        15.0,
        strictVersion,
        requiredVersion);

    if (!found.metaBase) return result;

    std::uint32_t totalSize = 0;
    if (!CalcTotalSizeFromHeader(*acc, found.metaBase, totalSize)) return result;

    return ReadMetadataRegion(*acc, found.metaBase, totalSize);
}

}

inline std::vector<std::uint8_t> ExportMetadataTScore()
{
    return detail_metadata::ExportMetadataImpl(0, false);
}

inline std::vector<std::uint8_t> ExportMetadataTVersion()
{
    std::uint32_t version = GetMetadataTargetVersion();
    if (version == 0)
    {
        return detail_metadata::ExportMetadataImpl(0, true);
    }
    return detail_metadata::ExportMetadataImpl(version, false);
}

inline std::vector<std::uint8_t> ExportMetadataThroughScore()
{
    return ExportMetadataTScore();
}

inline std::vector<std::uint8_t> ExportMetadataThroughVersion()
{
    return ExportMetadataTVersion();
}

}

#endif
