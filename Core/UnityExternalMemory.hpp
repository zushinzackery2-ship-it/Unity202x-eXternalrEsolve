#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace UnityExternal
{

// Memory accessor interface for cross-process memory reading
class IMemoryAccessor
{
public:
    virtual ~IMemoryAccessor() = default;
    virtual bool Read(std::uintptr_t address, void* buffer, std::size_t size) const = 0;
    virtual bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const = 0;
};

// Read a typed value from memory
template <typename T>
inline bool ReadValue(const IMemoryAccessor& mem, std::uintptr_t address, T& out)
{
    out = T{};
    if (!address) return false;
    return mem.Read(address, &out, sizeof(T));
}

// Read a pointer from memory
inline bool ReadPtr(const IMemoryAccessor& mem, std::uintptr_t address, std::uintptr_t& out)
{
    out = 0;
    if (!address) return false;
    return ReadValue(mem, address, out);
}

// Read a 32-bit integer from memory
inline bool ReadInt32(const IMemoryAccessor& mem, std::uintptr_t address, std::int32_t& out)
{
    out = 0;
    if (!address) return false;
    return ReadValue(mem, address, out);
}

// Read a C-string from memory (with max length limit)
inline bool ReadCString(const IMemoryAccessor& mem, std::uintptr_t address, std::string& out, std::size_t maxLen = 256)
{
    out.clear();
    if (!address)
    {
        return false;
    }

    if (maxLen == 0)
    {
        return false;
    }

    std::size_t readLen = maxLen;
    if (readLen > 4096) readLen = 4096;

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
        if (buffer[n] == '\0') break;
    }

    out.assign(buffer.data(), n);
    return true;
}

// Write a typed value to memory
template <typename T>
inline bool WriteValue(const IMemoryAccessor& mem, std::uintptr_t address, const T& value)
{
    return mem.Write(address, &value, sizeof(T));
}

} // namespace UnityExternal
