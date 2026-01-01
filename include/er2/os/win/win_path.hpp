#pragma once

#include <Windows.h>

#include <string>

namespace er2
{

inline std::string GetExeDirA()
{
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
    {
        return std::string();
    }

    for (DWORD i = n; i > 0; --i)
    {
        const char c = path[i - 1];
        if (c == '\\' || c == '/')
        {
            path[i - 1] = '\0';
            break;
        }
    }

    return std::string(path);
}

inline std::string JoinPathA(const std::string& a, const char* b)
{
    if (a.empty())
    {
        return std::string(b);
    }

    std::string out = a;
    const char last = out[out.size() - 1];
    if (last != '\\' && last != '/')
    {
        out.push_back('\\');
    }
    out += b;
    return out;
}

} // namespace er2
