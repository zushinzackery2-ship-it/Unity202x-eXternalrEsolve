#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <cstdint>
#include <vector>
#include <cwchar>

namespace er2
{

struct ModuleInfo
{
    std::uintptr_t base = 0;
    std::uint32_t size = 0;
};

inline bool GetRemoteModuleInfo(std::uint32_t pid, const wchar_t* moduleName, ModuleInfo& out)
{
    out = ModuleInfo{};

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<DWORD>(pid));
    if (snap == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    MODULEENTRY32W me;
    me.dwSize = sizeof(me);

    BOOL ok = Module32FirstW(snap, &me);
    while (ok)
    {
        if (_wcsicmp(me.szModule, moduleName) == 0)
        {
            out.base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
            out.size = static_cast<std::uint32_t>(me.modBaseSize);
            CloseHandle(snap);
            return true;
        }
        ok = Module32NextW(snap, &me);
    }

    CloseHandle(snap);
    return false;
}

inline std::uintptr_t FindModuleBase(std::uint32_t pid, const wchar_t* moduleName)
{
    if (!moduleName || moduleName[0] == L'\0')
    {
        return 0;
    }

    ModuleInfo mi;
    if (!GetRemoteModuleInfo(pid, moduleName, mi))
    {
        return 0;
    }

    return mi.base;
}

inline std::vector<std::uint32_t> FindPidsWithModule(const wchar_t* moduleName)
{
    std::vector<std::uint32_t> out;
    if (!moduleName || moduleName[0] == L'\0')
    {
        return out;
    }

    HANDLE psnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (psnap == INVALID_HANDLE_VALUE)
    {
        return out;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(psnap, &pe))
    {
        do
        {
            const std::uint32_t pid = static_cast<std::uint32_t>(pe.th32ProcessID);
            if (pid == 0)
            {
                continue;
            }

            HANDLE msnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<DWORD>(pid));
            if (msnap == INVALID_HANDLE_VALUE)
            {
                continue;
            }

            bool has = false;
            MODULEENTRY32W me;
            me.dwSize = sizeof(me);
            if (Module32FirstW(msnap, &me))
            {
                do
                {
                    if (_wcsicmp(me.szModule, moduleName) == 0)
                    {
                        has = true;
                        break;
                    }
                } while (Module32NextW(msnap, &me));
            }

            CloseHandle(msnap);

            if (has)
            {
                out.push_back(pid);
            }

            if (out.size() >= 64)
            {
                break;
            }

        } while (Process32NextW(psnap, &pe));
    }

    CloseHandle(psnap);

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

} // namespace er2
