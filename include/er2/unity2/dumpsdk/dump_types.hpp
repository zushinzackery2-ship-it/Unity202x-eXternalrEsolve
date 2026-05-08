#pragma once

#include <cstdint>
#include <vector>

#include "../../mem/memory_accessor.hpp"
#include "../../mem/memory_read.hpp"
#include "../metadata/registration/registration_helpers.hpp"

#include "sdk_common.hpp"
#include "sdk_metadata_helpers.hpp"

namespace er2
{

inline bool DumpSdk6TryRvaToFileOffset(std::uint32_t rva, const std::vector<detail_il2cpp_reg::DiskSection>& secs, std::uint64_t& out)
{
    out = 0;

    if (rva == 0)
    {
        return false;
    }

    for (const auto& s : secs)
    {
        if (s.vsize == 0)
        {
            continue;
        }

        const std::uint32_t start = s.rva;
        const std::uint32_t end = start + s.vsize;

        if (rva < start || rva >= end)
        {
            continue;
        }

        if (s.rawPtr == 0)
        {
            return false;
        }

        out = static_cast<std::uint64_t>(s.rawPtr) + static_cast<std::uint64_t>(rva - start);
        return true;
    }

    return false;
}

} // namespace er2
