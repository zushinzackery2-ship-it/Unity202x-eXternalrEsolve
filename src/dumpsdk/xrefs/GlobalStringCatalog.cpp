#include "GlobalStringCatalog.h"

#include "GlobalStringDecoder.h"
#include "XrefPeAccess.h"

#include <er2/unity2/dumpsdk/dump_progress.hpp>

#include <algorithm>

namespace er2
{

std::vector<DetectedGlobalString> GlobalStringCatalog::Extract(
    const PeImage& image,
    const GlobalStringXrefOptions& options)
{
    std::vector<DetectedGlobalString> result;
    std::uint64_t totalBytes = 0;
    for (const PeSectionHeader& section : image.Sections())
    {
        if (IsStringSection(section))
        {
            totalBytes += section.Span();
        }
    }
    DumpSdkProgressScope progress("Catalog native strings", totalBytes);
    std::uint64_t processedBytes = 0;
    constexpr std::size_t ProgressChunkSize = 64u * 1024u;

    for (const PeSectionHeader& section : image.Sections())
    {
        if (!IsStringSection(section))
        {
            continue;
        }

        const std::uint32_t sectionSize = section.Span();
        const std::uint8_t* bytes = XrefRvaToData(image, section.virtualAddress, sectionSize);
        if (bytes == nullptr)
        {
            processedBytes += sectionSize;
            progress.Update(processedBytes);
            continue;
        }

        std::size_t offset = 0;
        std::size_t nextProgressOffset = ProgressChunkSize;
        while (offset < sectionSize)
        {
            if (offset > 0 && bytes[offset - 1] != 0)
            {
                ++offset;
            }
            else
            {
                DecodedGlobalString decoded;
                if (!GlobalStringDecoder::TryDecode(
                        bytes + offset,
                        sectionSize - offset,
                        options,
                        decoded))
                {
                    ++offset;
                }
                else
                {
                    const std::uint32_t rva = section.virtualAddress
                        + static_cast<std::uint32_t>(offset);
                    result.push_back({
                        static_cast<std::uintptr_t>(image.ImageBase()) + rva,
                        rva,
                        decoded.byteLength,
                        XrefSectionName(section),
                        decoded.encoding,
                        decoded.value,
                        {} });
                    offset += decoded.byteLength;
                }
            }

            if (offset >= nextProgressOffset || offset == sectionSize)
            {
                progress.Update(processedBytes + offset);
                nextProgressOffset = offset + ProgressChunkSize;
            }
        }
        processedBytes += sectionSize;
        progress.Update(processedBytes);
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const DetectedGlobalString& left, const DetectedGlobalString& right)
        {
            return left.address < right.address;
        });
    progress.Complete();
    return result;
}

bool GlobalStringCatalog::TryDecodeAtAddress(
    const PeImage& image,
    std::uintptr_t address,
    const GlobalStringXrefOptions& options,
    DetectedGlobalString& detected)
{
    const PeSectionHeader* section = FindStringSection(image, address);
    if (section == nullptr)
    {
        return false;
    }

    const std::uintptr_t sectionAddress = static_cast<std::uintptr_t>(image.ImageBase())
        + section->virtualAddress;
    const std::size_t offset = static_cast<std::size_t>(address - sectionAddress);
    const std::size_t available = section->Span() - offset;
    const std::uint8_t* bytes = XrefRvaToData(
        image,
        section->virtualAddress + static_cast<std::uint32_t>(offset),
        available);
    if (bytes == nullptr)
    {
        return false;
    }

    DecodedGlobalString decoded;
    if (!GlobalStringDecoder::TryDecode(bytes, available, options, decoded))
    {
        return false;
    }

    const std::uint32_t rva = static_cast<std::uint32_t>(address - image.ImageBase());
    detected = {
        address,
        rva,
        decoded.byteLength,
        XrefSectionName(*section),
        decoded.encoding,
        decoded.value,
        {} };
    return true;
}

bool GlobalStringCatalog::FindContaining(
    const std::vector<DetectedGlobalString>& strings,
    std::uintptr_t address,
    std::size_t& index)
{
    std::size_t low = 0;
    std::size_t high = strings.size();
    while (low < high)
    {
        const std::size_t middle = low + (high - low) / 2;
        const DetectedGlobalString& item = strings[middle];
        if (address < item.address)
        {
            high = middle;
        }
        else if (address >= item.address + item.byteLength)
        {
            low = middle + 1;
        }
        else
        {
            index = middle;
            return true;
        }
    }
    return false;
}

bool GlobalStringCatalog::IsStringSection(const PeSectionHeader& section)
{
    const std::string name = XrefSectionName(section);
    return section.Span() > 0
        && (section.characteristics & kImageScnMemRead) != 0
        && (section.characteristics & kImageScnMemExecute) == 0
        && !XrefEqualsIgnoreCase(name, ".pdata")
        && !XrefEqualsIgnoreCase(name, ".xdata")
        && !XrefEqualsIgnoreCase(name, ".reloc");
}

const PeSectionHeader* GlobalStringCatalog::FindStringSection(
    const PeImage& image,
    std::uintptr_t address)
{
    for (const PeSectionHeader& section : image.Sections())
    {
        if (!IsStringSection(section))
        {
            continue;
        }

        const std::uintptr_t start = static_cast<std::uintptr_t>(image.ImageBase())
            + section.virtualAddress;
        if (address >= start && address < start + section.Span())
        {
            return &section;
        }
    }
    return nullptr;
}

} // namespace er2
