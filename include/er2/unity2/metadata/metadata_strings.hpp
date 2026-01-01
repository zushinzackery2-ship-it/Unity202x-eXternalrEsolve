#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "metadata_header_fields.hpp"

namespace er2
{

inline bool ReadCStringFromMetadataStrings(
    const IMemoryAccessor& mem,
    std::uintptr_t metaBase,
    const MetadataHeaderFields& h,
    std::int32_t nameIndex,
    std::string& out)
{
    out.clear();
    if (nameIndex < 0 || h.stringOffset == 0 || h.stringSize == 0)
    {
        return false;
    }

    const std::uint32_t idx = static_cast<std::uint32_t>(nameIndex);
    if (idx >= h.stringSize)
    {
        return false;
    }

    const std::uintptr_t p = metaBase + static_cast<std::uintptr_t>(h.stringOffset) + static_cast<std::uintptr_t>(idx);
    return ReadCString(mem, p, out, 260);
}

inline bool ReadCStringFromMetadataStringsBytes(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    std::int32_t nameIndex,
    std::string& out)
{
    out.clear();
    if (nameIndex < 0 || h.stringOffset == 0 || h.stringSize == 0)
    {
        return false;
    }

    const std::uint32_t idx = static_cast<std::uint32_t>(nameIndex);
    if (idx >= h.stringSize)
    {
        return false;
    }

    const std::size_t start = static_cast<std::size_t>(h.stringOffset) + static_cast<std::size_t>(idx);
    const std::size_t maxEnd = static_cast<std::size_t>(h.stringOffset) + static_cast<std::size_t>(h.stringSize);
    if (start >= meta.size())
    {
        return false;
    }

    std::size_t end = start;
    while (end < meta.size() && end < maxEnd)
    {
        if (meta[end] == 0)
        {
            break;
        }
        ++end;
    }

    out.assign(reinterpret_cast<const char*>(meta.data() + start), reinterpret_cast<const char*>(meta.data() + end));
    return !out.empty();
}

} // namespace er2
