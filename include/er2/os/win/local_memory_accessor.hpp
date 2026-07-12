#pragma once

#include <Windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../../mem/memory_accessor.hpp"

namespace er2
{

namespace detail_local_mem
{

inline bool RegionAllowsRead(DWORD protect)
{
    switch (protect & 0xFFu)
    {
    case PAGE_READONLY:
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

inline bool RegionAllowsWrite(DWORD protect)
{
    switch (protect & 0xFFu)
    {
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

inline bool AddressRangeWraps(std::uintptr_t address, std::size_t size)
{
    return size != 0 && address + size < address;
}

inline bool RangeCommitted(std::uintptr_t address, std::size_t size, bool (*allowProtect)(DWORD))
{
    if (address == 0 || size == 0 || AddressRangeWraps(address, size) || allowProtect == nullptr)
    {
        return false;
    }

    std::size_t remaining = size;
    std::uintptr_t cursor = address;
    while (remaining > 0)
    {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &info, sizeof(info)) == 0)
        {
            return false;
        }
        if (info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0 || !allowProtect(info.Protect))
        {
            return false;
        }

        const std::uintptr_t regionStart = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        const std::uintptr_t regionEnd = regionStart + info.RegionSize;
        if (cursor < regionStart || cursor >= regionEnd)
        {
            return false;
        }

        const std::size_t chunk = static_cast<std::size_t>((std::min)(regionEnd - cursor, static_cast<std::uintptr_t>(remaining)));
        remaining -= chunk;
        cursor += chunk;
    }
    return true;
}

inline int SehCopy(std::uintptr_t address, void* destination, std::size_t size)
{
    __try
    {
        std::memcpy(destination, reinterpret_cast<const void*>(address), size);
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

inline int SehWrite(std::uintptr_t address, const void* source, std::size_t size)
{
    __try
    {
        std::memcpy(reinterpret_cast<void*>(address), source, size);
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

} // namespace detail_local_mem

/// In-process IMemoryAccessor for injected / same-process callers.
/// Uses VirtualQuery range checks + SEH memcpy (no OpenProcess / RPM).
class LocalMemoryAccessor final : public IMemoryAccessor
{
public:
    bool Read(std::uintptr_t address, void* buffer, std::size_t size) const override
    {
        if (buffer == nullptr || !detail_local_mem::RangeCommitted(address, size, detail_local_mem::RegionAllowsRead))
        {
            return false;
        }
        return detail_local_mem::SehCopy(address, buffer, size) != 0;
    }

    bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const override
    {
        if (buffer == nullptr || !detail_local_mem::RangeCommitted(address, size, detail_local_mem::RegionAllowsWrite))
        {
            return false;
        }
        return detail_local_mem::SehWrite(address, buffer, size) != 0;
    }
};

} // namespace er2
