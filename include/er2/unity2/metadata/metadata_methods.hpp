#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "metadata_header_fields.hpp"

namespace er2
{

struct MethodDefLayout
{
    std::size_t stride = 0;
    std::size_t tokenOffset = 0;
    std::uint32_t count = 0;
};

struct MethodDefView
{
    std::uint32_t index = 0;
    std::int32_t nameIndex = -1;
    std::int32_t declaringType = -1;
    std::uint32_t token = 0;
};

namespace detail_metadata_methods
{

inline std::uint32_t ReadU32LE(const std::vector<std::uint8_t>& b, std::size_t off)
{
    if (off + 4 > b.size())
    {
        return 0;
    }
    return (std::uint32_t)b[off + 0] |
           ((std::uint32_t)b[off + 1] << 8) |
           ((std::uint32_t)b[off + 2] << 16) |
           ((std::uint32_t)b[off + 3] << 24);
}

inline bool IsLikelyMethodToken(std::uint32_t token)
{
    // MethodDef token is 0x06??????
    return (token & 0xFF000000u) == 0x06000000u && (token & 0x00FFFFFFu) != 0u;
}

inline bool TryParseAt(const std::vector<std::uint8_t>& meta, std::size_t base, std::size_t stride, std::size_t tokenOff, MethodDefView& out)
{
    out = MethodDefView{};
    if (base + stride > meta.size())
    {
        return false;
    }
    if (tokenOff + 4 > stride)
    {
        return false;
    }

    out.nameIndex = static_cast<std::int32_t>(ReadU32LE(meta, base + 0x00u));
    out.declaringType = static_cast<std::int32_t>(ReadU32LE(meta, base + 0x04u));
    out.token = ReadU32LE(meta, base + tokenOff);

    return IsLikelyMethodToken(out.token);
}

} // namespace detail_metadata_methods

inline bool DetectMethodDefLayout(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    MethodDefLayout& outLayout)
{
    outLayout = MethodDefLayout{};

    if (h.methodsOffset == 0 || h.methodsSize == 0)
    {
        return false;
    }

    struct Candidate
    {
        std::size_t stride;
        std::size_t tokenOff;
    };

    // Known: Unity 6000.2 (metadata version 31) has MethodDefinition stride 0x20, token at 0x14.
    // Older commonly: 0x24 with token at 0x18.
    const Candidate candidates[] = {
        {0x20u, 0x14u},
        {0x24u, 0x18u},
        {0x28u, 0x1Cu},
    };

    int bestScore = -1;

    for (const auto& c : candidates)
    {
        if ((h.methodsSize % static_cast<std::uint32_t>(c.stride)) != 0u)
        {
            continue;
        }

        const std::uint32_t cnt = h.methodsSize / static_cast<std::uint32_t>(c.stride);
        if (cnt == 0 || cnt > 5000000u)
        {
            continue;
        }

        const std::uint32_t sample = (cnt > 256u) ? 256u : cnt;
        int ok = 0;
        for (std::uint32_t i = 0; i < sample; ++i)
        {
            const std::size_t base = static_cast<std::size_t>(h.methodsOffset) + static_cast<std::size_t>(i) * c.stride;
            MethodDefView v;
            if (detail_metadata_methods::TryParseAt(meta, base, c.stride, c.tokenOff, v))
            {
                ++ok;
            }
        }

        if (ok > bestScore)
        {
            bestScore = ok;
            outLayout.stride = c.stride;
            outLayout.tokenOffset = c.tokenOff;
            outLayout.count = cnt;
        }
    }

    return outLayout.stride != 0 && outLayout.count != 0;
}

inline bool ReadMethodDefView(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    const MethodDefLayout& layout,
    std::uint32_t methodDefIndex,
    MethodDefView& out)
{
    out = MethodDefView{};
    out.index = methodDefIndex;

    if (layout.stride == 0 || h.methodsOffset == 0)
    {
        return false;
    }

    const std::size_t base = static_cast<std::size_t>(h.methodsOffset) + static_cast<std::size_t>(methodDefIndex) * layout.stride;
    if (base + layout.stride > meta.size())
    {
        return false;
    }

    // Always fill fields (even if token heuristic fails).
    out.nameIndex = static_cast<std::int32_t>(detail_metadata_methods::ReadU32LE(meta, base + 0x00u));
    out.declaringType = static_cast<std::int32_t>(detail_metadata_methods::ReadU32LE(meta, base + 0x04u));
    out.token = detail_metadata_methods::ReadU32LE(meta, base + layout.tokenOffset);

    return true;
}

} // namespace er2
