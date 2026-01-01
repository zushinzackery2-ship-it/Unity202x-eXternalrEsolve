
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Windows.h>

#include "../../mem/memory_read.hpp"

namespace er2
{

struct ModuleSection
{
    std::string name;
    std::uint32_t rva = 0;
    std::uint32_t size = 0;
};

inline bool ReadModuleImageBase(const IMemoryAccessor& mem, std::uintptr_t moduleBase, std::uint64_t& outImageBase)
{
    outImageBase = 0;

    IMAGE_DOS_HEADER dos{};
    if (!mem.Read(moduleBase, &dos, sizeof(dos)))
    {
        return false;
    }
    if (dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x1000)
    {
        return false;
    }

    IMAGE_NT_HEADERS64 nt{};
    if (!mem.Read(moduleBase + (std::uintptr_t)dos.e_lfanew, &nt, sizeof(nt)))
    {
        return false;
    }
    if (nt.Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }
    if (nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return false;
    }

    outImageBase = (std::uint64_t)nt.OptionalHeader.ImageBase;
    return outImageBase != 0;
}

inline bool ReadModuleSections(const IMemoryAccessor& mem, std::uintptr_t moduleBase, std::uint32_t& outSizeOfImage, std::vector<ModuleSection>& outSections)
{
    outSizeOfImage = 0;
    outSections.clear();

    IMAGE_DOS_HEADER dos{};
    if (!mem.Read(moduleBase, &dos, sizeof(dos)))
    {
        return false;
    }
    if (dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x1000)
    {
        return false;
    }

    IMAGE_NT_HEADERS64 nt{};
    if (!mem.Read(moduleBase + (std::uintptr_t)dos.e_lfanew, &nt, sizeof(nt)))
    {
        return false;
    }
    if (nt.Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }
    if (nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return false;
    }

    const std::uint16_t nsec = nt.FileHeader.NumberOfSections;
    if (nsec == 0 || nsec > 96)
    {
        return false;
    }

    outSizeOfImage = nt.OptionalHeader.SizeOfImage;
    if (outSizeOfImage == 0)
    {
        return false;
    }

    std::uintptr_t secBase = moduleBase + (std::uintptr_t)dos.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;

    outSections.reserve(nsec);
    for (std::uint16_t i = 0; i < nsec; ++i)
    {
        IMAGE_SECTION_HEADER sh{};
        if (!mem.Read(secBase + (std::uintptr_t)i * sizeof(IMAGE_SECTION_HEADER), &sh, sizeof(sh)))
        {
            return false;
        }

        char nameBuf[9] = {};
        for (int j = 0; j < 8; ++j)
        {
            nameBuf[j] = (char)sh.Name[j];
            if (nameBuf[j] == '\0')
            {
                break;
            }
        }

        ModuleSection s;
        s.name = nameBuf;
        s.rva = sh.VirtualAddress;
        s.size = sh.Misc.VirtualSize;
        outSections.push_back(s);
    }

    return true;
}

} // namespace er2
