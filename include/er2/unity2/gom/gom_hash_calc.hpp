#pragma once

#include <cstdint>

namespace er2
{

/// <summary>

/// </summary>
inline std::uint32_t CalHashmaskThrougTag(std::int32_t tag)
{
    const std::uint32_t t = static_cast<std::uint32_t>(tag);
    const std::uint32_t h = t ^ (t >> 16);
    return h;
}

} // namespace er2
