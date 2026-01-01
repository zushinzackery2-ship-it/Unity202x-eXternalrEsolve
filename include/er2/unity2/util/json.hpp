#pragma once

#include <string>

namespace er2
{

inline std::string JsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s)
    {
        switch (c)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

} // namespace er2
