#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>

#include "../../mem/memory_accessor.hpp"

namespace er2
{

class WinApiMemoryAccessor : public IMemoryAccessor
{
public:
    explicit WinApiMemoryAccessor(HANDLE process)
        : m_process(process)
    {
    }

    bool Read(std::uintptr_t address, void* buffer, std::size_t size) const override
    {
        if (!m_process || !address || !buffer || size == 0)
        {
            return false;
        }

        SIZE_T bytesRead = 0;
        return ReadProcessMemory(m_process, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead) != 0 && bytesRead == size;
    }

    bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const override
    {
        if (!m_process || !address || !buffer || size == 0)
        {
            return false;
        }

        SIZE_T bytesWritten = 0;
        return WriteProcessMemory(m_process, reinterpret_cast<LPVOID>(address), buffer, size, &bytesWritten) != 0 && bytesWritten == size;
    }

    HANDLE GetProcessHandle() const
    {
        return m_process;
    }

private:
    HANDLE m_process = nullptr;
};

} // namespace er2
