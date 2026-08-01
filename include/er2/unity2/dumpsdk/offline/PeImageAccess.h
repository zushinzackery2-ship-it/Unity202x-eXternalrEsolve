#pragma once

#include <er2/unity2/dumpsdk/offline/PeImage.h>

#include <Windows.h>
#include <Psapi.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace er2
{

struct PeSearchSection
{
    uint64_t offset = 0;
    uint64_t offsetEnd = 0;
    uint64_t address = 0;
    uint64_t addressEnd = 0;
};

inline PeSearchSection MakeSearchSection(const PeImage& pe, const PeSectionHeader& section)
{
    const uint64_t span = section.virtualSize != 0 ? section.virtualSize : section.sizeOfRawData;
  PeSearchSection range{};
    range.offset = section.pointerToRawData;
    range.offsetEnd = section.pointerToRawData + span;
    range.address = pe.ImageBase() + section.virtualAddress;
    range.addressEnd = range.address + span;
    return range;
}

inline std::vector<PeSearchSection> BuildExecSearchSections(const PeImage& pe)
{
    std::vector<PeSearchSection> sections;
    for (const PeSectionHeader& section : pe.ExecSections())
    {
        sections.push_back(MakeSearchSection(pe, section));
    }
    return sections;
}

inline std::vector<PeSearchSection> BuildDataSearchSections(const PeImage& pe)
{
    std::vector<PeSearchSection> sections;
    for (const PeSectionHeader& section : pe.DataSections())
    {
        sections.push_back(MakeSearchSection(pe, section));
    }
    return sections;
}

inline bool TryReadU64(const PeImage& pe, uint64_t absoluteAddress, uint64_t& out)
{
    try
    {
        out = pe.ReadU64(absoluteAddress);
        return true;
    }
    catch (...)
    {
        out = 0;
        return false;
    }
}

inline bool TryReadU32(const PeImage& pe, uint64_t absoluteAddress, uint32_t& out)
{
    try
    {
        const std::vector<uint8_t> bytes = pe.ReadBytes(absoluteAddress, sizeof(out));
        if (bytes.size() != sizeof(out))
        {
            out = 0;
            return false;
        }
        std::memcpy(&out, bytes.data(), sizeof(out));
        return true;
    }
    catch (...)
    {
        out = 0;
        return false;
    }
}

inline int64_t ReadIntPtrAbs(const PeImage& pe, uint64_t absoluteAddress)
{
    return static_cast<int64_t>(pe.ReadU64(absoluteAddress));
}

inline uint64_t ReadUIntPtrAbs(const PeImage& pe, uint64_t absoluteAddress)
{
    return pe.ReadU64(absoluteAddress);
}

inline bool ReadAbs(const PeImage& pe, uint64_t absoluteAddress, void* buffer, size_t size)
{
    try
    {
        const std::vector<uint8_t> bytes = pe.ReadBytes(absoluteAddress, size);
        if (bytes.size() != size)
        {
            return false;
        }
        std::memcpy(buffer, bytes.data(), size);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

inline bool IsPointerInSectionList(const PeImage& pe,
    const std::vector<PeSectionHeader>& sections,
    uint64_t pointer)
{
    if (pointer < pe.ImageBase())
    {
        return false;
    }
    const uint32_t rva = static_cast<uint32_t>(pointer - pe.ImageBase());
    for (const PeSectionHeader& section : sections)
    {
        if (section.ContainsRva(rva))
        {
            return true;
        }
    }
    return false;
}

inline bool IsPointerInExec(const PeImage& pe, uint64_t pointer)
{
    return IsPointerInSectionList(pe, pe.ExecSections(), pointer);
}

inline bool IsPointerInData(const PeImage& pe, uint64_t pointer)
{
    return IsPointerInSectionList(pe, pe.DataSections(), pointer);
}

inline bool IsPointerInBss(const PeImage& pe, uint64_t pointer)
{
    return IsPointerInData(pe, pointer);
}

inline bool IsOffsetInData(const PeImage& pe, uint64_t offset)
{
    const uint32_t fileOffset = static_cast<uint32_t>(offset);
    for (const PeSectionHeader& section : pe.DataSections())
    {
        if (section.ContainsOffset(fileOffset))
        {
            return true;
        }
    }
    return false;
}

inline bool LoadPeImageFromModule(void* module, PeImage& out, std::string& error)
{
    return out.LoadFromModuleSnapshot(static_cast<HMODULE>(module), error);
}

inline bool LoadPeImageFromModuleRange(
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    PeImage& out,
    std::string& error)
{
    return out.LoadFromModuleRange(moduleBase, static_cast<size_t>(moduleSize), error);
}

} // namespace er2
