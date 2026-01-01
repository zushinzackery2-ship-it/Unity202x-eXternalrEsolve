#pragma once

#include <cstddef>
#include <string>

#include "ascii.hpp"

namespace er2
{

inline std::string BaseNameNoExt(std::string s)
{
    const std::size_t p0 = s.find_last_of("/\\");
    if (p0 != std::string::npos)
    {
        s = s.substr(p0 + 1);
    }

    const std::string low = ToLowerAscii(s);
    if (low.size() >= 4 && low.compare(low.size() - 4, 4, ".dll") == 0)
    {
        s.resize(s.size() - 4);
    }
    return s;
}

inline std::string NormalizeAssemblyKey(const std::string& name)
{
    return ToLowerAscii(BaseNameNoExt(name));
}

} // namespace er2
