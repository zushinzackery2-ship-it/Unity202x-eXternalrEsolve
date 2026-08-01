#include <er2/unity2/dumpsdk/offline/PeImage.h>

#include <stdexcept>

namespace er2
{

namespace
{

bool IsExecSection(uint32_t characteristics)
{
    return (characteristics & kImageScnMemExecute) != 0 &&
        (characteristics & kImageScnCntCode) != 0;
}

bool IsDataSection(uint32_t characteristics)
{
    return (characteristics & kImageScnMemRead) != 0 &&
        (characteristics & (kImageScnCntInitializedData | kImageScnCntUninitializedData)) != 0;
}

} // namespace

const PeSectionHeader* PeImage::FindSectionByRva(uint32_t rva) const
{
    for (const PeSectionHeader& section : sections_)
    {
        if (section.ContainsRva(rva))
        {
            return &section;
        }
    }
    return nullptr;
}

const PeSectionHeader* PeImage::FindSectionByOffset(uint32_t offset) const
{
    for (const PeSectionHeader& section : sections_)
    {
        if (section.ContainsOffset(offset))
        {
            return &section;
        }
    }
    return nullptr;
}

const PeSectionHeader* PeImage::FindDiskSectionByRva(uint32_t rva) const
{
    for (const PeSectionHeader& section : diskSections_)
    {
        if (section.ContainsRva(rva))
        {
            return &section;
        }
    }
    return nullptr;
}

uint64_t PeImage::MapVATR(uint64_t absoluteAddress) const
{
    if (!memoryLoaded_)
    {
        throw std::runtime_error("LoadFromMemory must be called before MapVATR");
    }
    if (absoluteAddress < imageBase_)
    {
        return 0;
    }
    const uint64_t rva64 = absoluteAddress - imageBase_;
    if (rva64 > UINT32_MAX)
    {
        return 0;
    }
    const uint32_t rva = static_cast<uint32_t>(rva64);
    const PeSectionHeader* section = FindSectionByRva(rva);
    if (section == nullptr)
    {
        return 0;
    }
    return static_cast<uint64_t>(rva - section->virtualAddress) + section->pointerToRawData;
}

uint64_t PeImage::MapRTVA(uint64_t offset) const
{
    if (!memoryLoaded_)
    {
        throw std::runtime_error("LoadFromMemory must be called before MapRTVA");
    }
    if (offset > UINT32_MAX)
    {
        return 0;
    }
    const uint32_t fileOffset = static_cast<uint32_t>(offset);
    const PeSectionHeader* section = FindSectionByOffset(fileOffset);
    if (section == nullptr)
    {
        return 0;
    }
    return static_cast<uint64_t>(fileOffset - section->pointerToRawData) +
        section->virtualAddress + imageBase_;
}

uint64_t PeImage::MapFileOffset(uint64_t absoluteAddress) const
{
    if (absoluteAddress < imageBase_)
    {
        return 0;
    }
    const uint64_t rva64 = absoluteAddress - imageBase_;
    if (rva64 > UINT32_MAX)
    {
        return 0;
    }
    const uint32_t rva = static_cast<uint32_t>(rva64);
    const PeSectionHeader* section = FindDiskSectionByRva(rva);
    if (section == nullptr)
    {
        return 0;
    }
    return static_cast<uint64_t>(rva - section->virtualAddress) + section->pointerToRawData;
}

uint64_t PeImage::ReadU64(uint64_t absoluteAddress) const
{
    const uint64_t offset = MapVATR(absoluteAddress);
    if (offset == 0 && absoluteAddress != imageBase_)
    {
        throw StreamBoundsError("MapVATR failed for ReadU64");
    }
    BinaryStream reader(Data(), Size());
    reader.SetPosition(static_cast<size_t>(offset));
    return reader.ReadUInt64();
}

std::vector<uint8_t> PeImage::ReadBytes(uint64_t absoluteAddress, size_t count) const
{
    const uint64_t offset = MapVATR(absoluteAddress);
    if (offset == 0 && absoluteAddress != imageBase_)
    {
        throw StreamBoundsError("MapVATR failed for ReadBytes");
    }
    BinaryStream reader(Data(), Size());
    reader.SetPosition(static_cast<size_t>(offset));
    return reader.ReadBytes(count);
}

std::string PeImage::ReadCString(uint64_t absoluteAddress) const
{
    const uint64_t offset = MapVATR(absoluteAddress);
    if (offset == 0 && absoluteAddress != imageBase_)
    {
        throw StreamBoundsError("MapVATR failed for ReadCString");
    }
    BinaryStream reader(Data(), Size());
    reader.SetPosition(static_cast<size_t>(offset));
    return reader.ReadStringToNull();
}

std::vector<PeSectionHeader> PeImage::ExecSections() const
{
    std::vector<PeSectionHeader> result;
    for (const PeSectionHeader& section : sections_)
    {
        if (IsExecSection(section.characteristics))
        {
            result.push_back(section);
        }
    }
    return result;
}

std::vector<PeSectionHeader> PeImage::DataSections() const
{
    std::vector<PeSectionHeader> result;
    for (const PeSectionHeader& section : sections_)
    {
        if (IsDataSection(section.characteristics))
        {
            result.push_back(section);
        }
    }
    return result;
}

} // namespace er2
