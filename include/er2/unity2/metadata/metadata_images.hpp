#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "metadata_header_fields.hpp"
#include "metadata_strings.hpp"

namespace er2
{

struct MetadataImageInfo
{
    std::uint32_t index = 0;
    std::string name;
    std::uint32_t typeStart = 0;
    std::uint32_t typeCount = 0;
};

namespace detail_metadata_images
{
}

inline bool ReadImagesFromMemory(
    const IMemoryAccessor& mem,
    std::uintptr_t metaBase,
    const MetadataHeaderFields& h,
    std::vector<MetadataImageInfo>& out)
{
    out.clear();
    if (!metaBase || h.imagesOffset == 0 || h.imagesSize == 0)
    {
        return false;
    }
    if ((h.imagesSize % 0x28u) != 0u)
    {
        return false;
    }

    const std::uint32_t count = h.imagesSize / 0x28u;
    if (count == 0 || count > 300000u)
    {
        return false;
    }

    out.reserve(count);

    for (std::uint32_t i = 0; i < count; ++i)
    {
        const std::uintptr_t base = metaBase + static_cast<std::uintptr_t>(h.imagesOffset) + static_cast<std::uintptr_t>(i) * 0x28u;

        std::int32_t nameIndex = 0;
        std::int32_t typeStartI = 0;
        std::int32_t typeCountI = 0;

        if (!ReadValue(mem, base + 0x00u, nameIndex) || !ReadValue(mem, base + 0x08u, typeStartI) || !ReadValue(mem, base + 0x0Cu, typeCountI))
        {
            continue;
        }
        if (typeStartI < 0 || typeCountI <= 0)
        {
            continue;
        }

        MetadataImageInfo info;
        info.index = i;
        info.typeStart = static_cast<std::uint32_t>(typeStartI);
        info.typeCount = static_cast<std::uint32_t>(typeCountI);
        (void)er2::ReadCStringFromMetadataStrings(mem, metaBase, h, nameIndex, info.name);
        out.push_back(std::move(info));
    }

    return !out.empty();
}

inline bool ReadImagesFromBytes(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    std::vector<MetadataImageInfo>& out)
{
    out.clear();
    if (h.imagesOffset == 0 || h.imagesSize == 0)
    {
        return false;
    }
    if ((h.imagesSize % 0x28u) != 0u)
    {
        return false;
    }

    const std::uint32_t count = h.imagesSize / 0x28u;
    if (count == 0 || count > 300000u)
    {
        return false;
    }

    const std::size_t endOff = static_cast<std::size_t>(h.imagesOffset) + static_cast<std::size_t>(h.imagesSize);
    if (endOff > meta.size())
    {
        return false;
    }

    out.reserve(count);

    for (std::uint32_t i = 0; i < count; ++i)
    {
        const std::size_t base = static_cast<std::size_t>(h.imagesOffset) + static_cast<std::size_t>(i) * 0x28u;

        // nameIndex @0x00, typeStart @0x08, typeCount @0x0C
        const std::int32_t nameIndex = static_cast<std::int32_t>(detail_metadata_header::ReadU32LE(meta.data() + base + 0x00u));
        const std::int32_t typeStartI = static_cast<std::int32_t>(detail_metadata_header::ReadU32LE(meta.data() + base + 0x08u));
        const std::int32_t typeCountI = static_cast<std::int32_t>(detail_metadata_header::ReadU32LE(meta.data() + base + 0x0Cu));

        if (typeStartI < 0 || typeCountI <= 0)
        {
            continue;
        }

        MetadataImageInfo info;
        info.index = i;
        info.typeStart = static_cast<std::uint32_t>(typeStartI);
        info.typeCount = static_cast<std::uint32_t>(typeCountI);
        (void)er2::ReadCStringFromMetadataStringsBytes(meta, h, nameIndex, info.name);
        out.push_back(std::move(info));
    }

    return !out.empty();
}

inline bool BuildTypeDefIndexToImageNameMap(
    const std::vector<MetadataImageInfo>& images,
    std::vector<std::string>& outTypeToImage)
{
    outTypeToImage.clear();
    if (images.empty())
    {
        return false;
    }

    std::uint32_t maxEnd = 0;
    for (const auto& img : images)
    {
        const std::uint32_t end = img.typeStart + img.typeCount;
        if (end > maxEnd)
        {
            maxEnd = end;
        }
    }
    if (maxEnd == 0)
    {
        return false;
    }

    outTypeToImage.assign(maxEnd, std::string());
    for (const auto& img : images)
    {
        const std::uint32_t start = img.typeStart;
        const std::uint32_t end = img.typeStart + img.typeCount;
        if (start >= outTypeToImage.size())
        {
            continue;
        }
        const std::uint32_t clampedEnd = end > outTypeToImage.size() ? static_cast<std::uint32_t>(outTypeToImage.size()) : end;
        for (std::uint32_t t = start; t < clampedEnd; ++t)
        {
            outTypeToImage[t] = img.name;
        }
    }

    return true;
}

inline bool BuildTypeDefIndexToImageNameFromMemory(
    const IMemoryAccessor& mem,
    std::uintptr_t metaBase,
    std::vector<std::string>& outTypeToImage)
{
    outTypeToImage.clear();

    MetadataHeaderFields h;
    if (!ReadMetadataHeaderFieldsFromMemory(mem, metaBase, h))
    {
        return false;
    }

    std::vector<MetadataImageInfo> imgs;
    if (!ReadImagesFromMemory(mem, metaBase, h, imgs))
    {
        return false;
    }

    return BuildTypeDefIndexToImageNameMap(imgs, outTypeToImage);
}

inline bool BuildTypeDefIndexToImageNameFromBytes(
    const std::vector<std::uint8_t>& meta,
    std::vector<std::string>& outTypeToImage)
{
    outTypeToImage.clear();

    MetadataHeaderFields h;
    if (!ReadMetadataHeaderFieldsFromBytes(meta, h))
    {
        return false;
    }

    std::vector<MetadataImageInfo> imgs;
    if (!ReadImagesFromBytes(meta, h, imgs))
    {
        return false;
    }

    return BuildTypeDefIndexToImageNameMap(imgs, outTypeToImage);
}

} // namespace er2
