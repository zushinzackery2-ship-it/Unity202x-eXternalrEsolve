#pragma once

#include <cstddef>
#include <cstdint>

namespace er2
{

/// <summary>

/// </summary>
class IMemoryAccessor
{
public:
    virtual ~IMemoryAccessor() = default;

    /// <summary>
    
    /// </summary>
    virtual bool Read(std::uintptr_t address, void* buffer, std::size_t size) const = 0;

    /// <summary>
    
    /// </summary>
    virtual bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const = 0;
};

} // namespace er2
