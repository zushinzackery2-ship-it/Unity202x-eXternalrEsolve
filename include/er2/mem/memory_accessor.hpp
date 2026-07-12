#pragma once

#include <cstddef>
#include <cstdint>

namespace er2
{

class IMemoryAccessor
{
public:
    virtual ~IMemoryAccessor() = default;

    virtual bool Read(std::uintptr_t address, void* buffer, std::size_t size) const = 0;

    virtual bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const = 0;
};

} // namespace er2
