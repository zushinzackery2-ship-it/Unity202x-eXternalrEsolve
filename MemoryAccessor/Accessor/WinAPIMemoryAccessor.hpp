#pragma once

#include <windows.h>
#include <cstdint>
#include <cstddef>

#include "../../Core/UnityExternalMemory.hpp"

namespace UnityExternal
{

struct WinAPIMemoryAccessor : IMemoryAccessor
{
    HANDLE process;

    explicit WinAPIMemoryAccessor(HANDLE hProcess = GetCurrentProcess())
        : process(hProcess) {}

    bool Read(std::uintptr_t address, void* buffer, std::size_t size) const override
    {
        if (!process || !buffer || size == 0)
        {
            return false;
        }

        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(process,
                               reinterpret_cast<LPCVOID>(address),
                               buffer,
                               static_cast<SIZE_T>(size),
                               &bytesRead))
        {
            return false;
        }

        return bytesRead == size;
    }

    bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const override
    {
        if (!process || !buffer || size == 0)
        {
            return false;
        }

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(process,
                                reinterpret_cast<LPVOID>(address),
                                buffer,
                                static_cast<SIZE_T>(size),
                                &bytesWritten))
        {
            return false;
        }

        return bytesWritten == size;
    }
};

} // namespace UnityExternal
