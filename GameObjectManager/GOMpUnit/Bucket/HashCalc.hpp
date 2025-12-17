#pragma once

#include <cstdint>

namespace UnityExternal
{

inline std::uint32_t CalHashmaskThrougTag(std::int32_t tagId)
{
    std::uint32_t searchKey = static_cast<std::uint32_t>(tagId);

    std::uint32_t t = 4097u * searchKey + 2127912214u;
    std::uint32_t hash1 = t ^ (t >> 19) ^ 0xC761C23Cu;

    std::uint32_t x = 33u * hash1;
    std::uint32_t y = 9u * (((x + 374761393u) << 9) ^ (x - 369570787u)) - 42973499u;

    std::uint32_t finalHash = (y ^ (y >> 16) ^ 0xB55A4F09u);
    std::uint32_t hashMask = finalHash & 0xFFFFFFFCu;

    return hashMask;
}

inline std::uint32_t CalTagThroughHashmask(std::int32_t tagId)
{
    return CalHashmaskThrougTag(tagId);
}

} // namespace UnityExternal
