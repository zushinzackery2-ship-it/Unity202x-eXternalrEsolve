#pragma once

#include "memory_accessor.hpp"
#include "../core/types.hpp"
#include <string>
#include <vector>
#include <cstring>

namespace er2
{

/// <summary>

/// </summary>
template<typename T>
inline bool ReadValue(const IMemoryAccessor& mem, std::uintptr_t address, T& out)
{
    out = T{};
    if (!address)
    {
        return false;
    }
    return mem.Read(address, &out, sizeof(T));
}

/// <summary>

/// </summary>
inline bool ReadPtr(const IMemoryAccessor& mem, std::uintptr_t address, std::uintptr_t& out)
{
    out = 0;
    if (!address)
    {
        return false;
    }
    return ReadValue(mem, address, out);
}

/// <summary>

/// </summary>
inline bool ReadInt32(const IMemoryAccessor& mem, std::uintptr_t address, std::int32_t& out)
{
    out = 0;
    if (!address)
    {
        return false;
    }
    return ReadValue(mem, address, out);
}

/// <summary>

/// </summary>
inline bool ReadCString(const IMemoryAccessor& mem, std::uintptr_t address, std::string& out, std::size_t maxLen = 256)
{
    out.clear();
    if (!address || maxLen == 0)
    {
        return false;
    }

    std::size_t readLen = maxLen;
    if (readLen > 4096)
    {
        readLen = 4096;
    }

    std::vector<char> buffer;
    buffer.resize(readLen + 1);

    if (!mem.Read(address, buffer.data(), readLen))
    {
        return false;
    }

    buffer[readLen] = '\0';
    std::size_t n = 0;
    for (; n < readLen; ++n)
    {
        if (buffer[n] == '\0')
        {
            break;
        }
    }

    out.assign(buffer.data(), n);
    return true;
}

/// <summary>

/// </summary>
template<typename T>
inline bool WriteValue(const IMemoryAccessor& mem, std::uintptr_t address, const T& value)
{
    return mem.Write(address, &value, sizeof(T));
}

/// <summary>

/// </summary>
template<std::size_t N>
inline bool ReadBytes(const IMemoryAccessor& mem, std::uintptr_t address, std::uint8_t (&buffer)[N])
{
    return mem.Read(address, buffer, N);
}

} // namespace er2
