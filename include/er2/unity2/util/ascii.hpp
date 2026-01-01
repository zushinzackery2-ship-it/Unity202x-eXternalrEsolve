#pragma once

#include <string>

namespace er2
{

inline std::string ToLowerAscii(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (char c : s)
    {
        if (c >= 'A' && c <= 'Z')
        {
            result.push_back(static_cast<char>(c - 'A' + 'a'));
        }
        else
        {
            result.push_back(c);
        }
    }
    return result;
}

inline std::string ToUpperAscii(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (char c : s)
    {
        if (c >= 'a' && c <= 'z')
        {
            result.push_back(static_cast<char>(c - 'a' + 'A'));
        }
        else
        {
            result.push_back(c);
        }
    }
    return result;
}

} // namespace er2
