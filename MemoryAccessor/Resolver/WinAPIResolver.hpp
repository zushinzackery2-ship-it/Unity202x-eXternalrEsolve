#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <atomic>

namespace UnityExternal
{

using ProcessIdResolver = DWORD(*)(const wchar_t* processName);
using ModuleBaseResolver = std::uintptr_t(*)(DWORD pid, const wchar_t* moduleName);

inline std::atomic<ProcessIdResolver> g_processIdResolver{nullptr};
inline std::atomic<ModuleBaseResolver> g_moduleBaseResolver{nullptr};

inline void SetProcessIdResolver(ProcessIdResolver resolver)
{
    g_processIdResolver.store(resolver, std::memory_order_release);
}

inline void SetModuleBaseResolver(ModuleBaseResolver resolver)
{
    g_moduleBaseResolver.store(resolver, std::memory_order_release);
}

inline DWORD WinAPI_FindProcessId(const wchar_t* processName)
{
    if (!processName || processName[0] == L'\0') return 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, processName) == 0)
            {
                DWORD pid = entry.th32ProcessID;
                CloseHandle(snapshot);
                return pid;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return 0;
}

inline std::uintptr_t WinAPI_FindModuleBase(DWORD pid, const wchar_t* moduleName)
{
    if (!moduleName || moduleName[0] == L'\0') return 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    if (Module32FirstW(snapshot, &me))
    {
        do
        {
            if (_wcsicmp(me.szModule, moduleName) == 0)
            {
                std::uintptr_t base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
                CloseHandle(snapshot);
                return base;
            }
        } while (Module32NextW(snapshot, &me));
    }
    CloseHandle(snapshot);
    return 0;
}

inline void UseWinAPIResolvers()
{
    SetProcessIdResolver(&WinAPI_FindProcessId);
    SetModuleBaseResolver(&WinAPI_FindModuleBase);
}

inline DWORD FindProcessId(const wchar_t* processName)
{
    ProcessIdResolver resolver = g_processIdResolver.load(std::memory_order_acquire);
    if (!resolver) return 0;
    return resolver(processName);
}

inline std::uintptr_t FindModuleBase(DWORD pid, const wchar_t* moduleName)
{
    ModuleBaseResolver resolver = g_moduleBaseResolver.load(std::memory_order_acquire);
    if (!resolver) return 0;
    return resolver(pid, moduleName);
}

} // namespace UnityExternal
