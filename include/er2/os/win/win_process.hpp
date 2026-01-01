#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#include <cstdint>
#include <cwchar>

namespace er2
{

inline HANDLE OpenProcessForRead(std::uint32_t pid)
{
    return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
}

inline HANDLE OpenProcessForReadWrite(std::uint32_t pid)
{
    return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, static_cast<DWORD>(pid));
}

inline std::uint32_t FindProcessId(const wchar_t* processName)
{
    if (!processName || processName[0] == L'\0')
    {
        return 0;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, processName) == 0)
            {
                const std::uint32_t pid = static_cast<std::uint32_t>(entry.th32ProcessID);
                CloseHandle(snapshot);
                return pid;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

} // namespace er2
