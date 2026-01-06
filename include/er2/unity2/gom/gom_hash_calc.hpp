#pragma once

#include <cstdint>

namespace er2
{

/// <summary>
/// Unity GameObjectManager bucket hash calculation.
/// Based on reverse-engineered core::base_hash_map::find function.
/// </summary>
inline std::uint32_t CalHashmaskThrougTag(std::int32_t tag)
{
    // Round 1: (4097 * key + 2127912214) ^ (>>19) ^ 0xC761C23C
    std::uint32_t hash1 = static_cast<std::uint32_t>(4097 * tag + 2127912214);
    hash1 = hash1 ^ (hash1 >> 19) ^ 0xC761C23C;
    
    // Round 2: complex mixing
    std::uint32_t a = 33 * hash1 + 374761393;
    std::uint32_t b = 33 * hash1 - 369570787;  // equivalent to 33 * hash1 + 0xE9FC4E7D (unsigned)
    std::uint32_t c = (a << 9) ^ b;
    std::uint32_t d = 9 * c - 42973499;        // equivalent to 9 * c + 0xFD7046C5 (unsigned)
    
    // Final: (d ^ (d >> 16) ^ 0xB55A4F09) & 0xFFFFFFFC
    std::uint32_t hashMask = (d ^ (d >> 16) ^ 0xB55A4F09) & 0xFFFFFFFC;
    
    return hashMask;
}

} // namespace er2

