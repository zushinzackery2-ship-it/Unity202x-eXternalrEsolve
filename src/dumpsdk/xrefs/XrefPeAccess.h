#pragma once

#include <er2/unity2/dumpsdk/offline/PeImage.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

namespace er2
{

inline std::string XrefSectionName(const PeSectionHeader& section)
{
    return section.name;
}

inline bool XrefEqualsIgnoreCase(const std::string& left, const char* right)
{
    std::size_t index = 0;
    while (index < left.size() && right[index] != '\0')
    {
        if (std::tolower(static_cast<unsigned char>(left[index]))
            != std::tolower(static_cast<unsigned char>(right[index])))
        {
            return false;
        }
        ++index;
    }
    return index == left.size() && right[index] == '\0';
}

inline const std::uint8_t* XrefRvaToData(
    const PeImage& image,
    std::uint32_t rva,
    std::size_t size)
{
    std::size_t offset = rva;
    if (!image.IsMemoryLoaded())
    {
        bool mapped = false;
        for (const PeSectionHeader& section : image.Sections())
        {
            if (!section.ContainsRva(rva))
            {
                continue;
            }
            offset = section.pointerToRawData + (rva - section.virtualAddress);
            mapped = true;
            break;
        }
        if (!mapped)
        {
            return nullptr;
        }
    }

    if (offset > image.Size() || size > image.Size() - offset)
    {
        return nullptr;
    }
    return image.Data() + offset;
}

} // namespace er2
