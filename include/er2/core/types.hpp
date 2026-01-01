#pragma once

#include <cstdint>
#include <cstddef>

namespace er2
{


using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using uptr = std::uintptr_t;


inline bool IsCanonicalUserPtr(std::uintptr_t ptr)
{
    if (ptr == 0)
    {
        return false;
    }

    
    constexpr std::uintptr_t kUserMin = 0x0000000000010000ull;
    constexpr std::uintptr_t kUserMax = 0x00007FFFFFFFFFFFull;

    return ptr >= kUserMin && ptr <= kUserMax;
}

inline bool IsLikelyPtr(std::uintptr_t ptr)
{
    return IsCanonicalUserPtr(ptr);
}

} // namespace er2
