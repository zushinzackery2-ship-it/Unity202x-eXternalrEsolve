#pragma once

#include "../../../os/win/win_include.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace er2
{

namespace detail_il2cpp_reg
{

inline bool EqualsIgnoreCase(const char* a, const char* b)
{
    if (!a || !b)
    {
        return false;
    }
    while (*a && *b)
    {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z')
        {
            ca = static_cast<char>(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z')
        {
            cb = static_cast<char>(cb - 'A' + 'a');
        }
        if (ca != cb)
        {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == 0 && *b == 0;
}

inline bool ContainsDllCaseInsensitive(const std::string& s)
{
    for (std::size_t i = 0; i + 3 < s.size(); ++i)
    {
        char c0 = s[i + 0];
        char c1 = s[i + 1];
        char c2 = s[i + 2];
        char c3 = s[i + 3];
        if (c0 >= 'A' && c0 <= 'Z') c0 = static_cast<char>(c0 - 'A' + 'a');
        if (c1 >= 'A' && c1 <= 'Z') c1 = static_cast<char>(c1 - 'A' + 'a');
        if (c2 >= 'A' && c2 <= 'Z') c2 = static_cast<char>(c2 - 'A' + 'a');
        if (c3 >= 'A' && c3 <= 'Z') c3 = static_cast<char>(c3 - 'A' + 'a');
        if (c0 == '.' && c1 == 'd' && c2 == 'l' && c3 == 'l')
        {
            return true;
        }
    }
    return false;
}

inline bool InAny(std::uintptr_t addr, const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& ranges)
{
    for (const auto& r : ranges)
    {
        if (addr >= r.first && addr < r.second)
        {
            return true;
        }
    }
    return false;
}

inline std::uint64_t U64At(const std::vector<std::uint8_t>& buf, std::size_t off)
{
    std::uint64_t v = 0;
    if (off + sizeof(v) <= buf.size())
    {
        std::memcpy(&v, buf.data() + off, sizeof(v));
    }
    return v;
}

inline std::int64_t I64At(const std::vector<std::uint8_t>& buf, std::size_t off)
{
    std::int64_t v = 0;
    if (off + sizeof(v) <= buf.size())
    {
        std::memcpy(&v, buf.data() + off, sizeof(v));
    }
    return v;
}

}

}
