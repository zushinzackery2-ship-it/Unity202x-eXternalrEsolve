#pragma once

#include <cstdint>
#include <vector>

#include "header_parser.hpp"
#include "scanner_pointer.hpp"

namespace er2
{

inline bool TryCalcMetadataTotalSizeByScore(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::size_t scanChunkSize,
    std::size_t maxPages,
    double maxSeconds,
    bool strictVersion,
    std::uint32_t requiredVersion,
    FoundMetadata& outFound,
    std::uint32_t& outTotalSize)
{
    outFound = FoundMetadata{};
    outTotalSize = 0;

    const FoundMetadata found = FindMetadataPointerByScore(
        mem,
        moduleBase,
        scanChunkSize,
        maxPages,
        maxSeconds,
        strictVersion,
        requiredVersion);

    if (!found.metaBase)
    {
        return false;
    }

    std::uint32_t totalSize = 0;
    if (!CalcTotalSizeFromHeader(mem, found.metaBase, totalSize))
    {
        return false;
    }

    outFound = found;
    outTotalSize = totalSize;
    return true;
}

inline bool ReadMetadataRegion(
    const IMemoryAccessor& mem,
    std::uintptr_t base,
    std::uint32_t size,
    std::size_t chunkSize,
    std::vector<std::uint8_t>& out)
{
    out.clear();
    if (!base || size == 0)
    {
        return false;
    }
    if (chunkSize == 0)
    {
        return false;
    }

    out.reserve(size);

    std::vector<std::uint8_t> buf;
    buf.resize(chunkSize);

    std::uint32_t remaining = size;
    std::uint32_t offset = 0;

    while (remaining > 0)
    {
        const std::size_t toRead = remaining > (std::uint32_t)buf.size() ? buf.size() : remaining;
        if (!mem.Read(base + offset, buf.data(), toRead))
        {
            out.clear();
            return false;
        }

        out.insert(out.end(), buf.begin(), buf.begin() + toRead);
        offset += (std::uint32_t)toRead;
        remaining -= (std::uint32_t)toRead;
    }

    return true;
}

inline bool ExportMetadataByScore(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::size_t scanChunkSize,
    std::size_t maxPages,
    double maxSeconds,
    bool strictVersion,
    std::uint32_t requiredVersion,
    std::size_t readChunkSize,
    std::vector<std::uint8_t>& out)
{
    out.clear();

    const FoundMetadata found = FindMetadataPointerByScore(
        mem,
        moduleBase,
        scanChunkSize,
        maxPages,
        maxSeconds,
        strictVersion,
        requiredVersion);

    if (!found.metaBase)
    {
        return false;
    }

    std::uint32_t totalSize = 0;
    if (!CalcTotalSizeFromHeader(mem, found.metaBase, totalSize))
    {
        return false;
    }

    return ReadMetadataRegion(mem, found.metaBase, totalSize, readChunkSize, out);
}

inline FoundMetadata FindMetadataByScore(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::size_t scanChunkSize,
    std::size_t maxPages,
    double maxSeconds,
    bool strictVersion,
    std::uint32_t requiredVersion)
{
    return FindMetadataPointerByScore(
        mem,
        moduleBase,
        scanChunkSize,
        maxPages,
        maxSeconds,
        strictVersion,
        requiredVersion);
}

} // namespace er2
