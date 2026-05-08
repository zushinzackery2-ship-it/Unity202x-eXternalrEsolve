#pragma once

#include <cstdint>
#include <vector>

#include "../../../mem/memory_read.hpp"

namespace er2
{

namespace detail_il2cpp_reg
{

inline bool ReadChunk(const IMemoryAccessor& mem, std::uintptr_t addr, std::size_t size, std::vector<std::uint8_t>& out)
{
    out.clear();
    if (size == 0)
    {
        return false;
    }
    out.resize(size);
    if (!mem.Read(addr, out.data(), size))
    {
        out.clear();
        return false;
    }
    return true;
}

}

}
