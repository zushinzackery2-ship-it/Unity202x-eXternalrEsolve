#pragma once

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "../../Core/UnityExternalMemory.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

struct ModuleSection
{
    char name[9];
    std::uint32_t rva;
    std::uint32_t size;
};

inline bool ReadModuleSections(const IMemoryAccessor& mem,
                               std::uintptr_t moduleBase,
                               std::uint32_t& outSizeOfImage,
                               std::vector<ModuleSection>& outSections)
{
    outSizeOfImage = 0;
    outSections.clear();

    IMAGE_DOS_HEADER dos{};
    if (!ReadValue(mem, moduleBase, dos)) return false;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) return false;
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x2000) return false;

    DWORD sig = 0;
    if (!ReadValue(mem, moduleBase + (std::uintptr_t)dos.e_lfanew, sig)) return false;
    if (sig != IMAGE_NT_SIGNATURE) return false;

    IMAGE_FILE_HEADER fh{};
    if (!ReadValue(mem, moduleBase + (std::uintptr_t)dos.e_lfanew + 4u, fh)) return false;
    std::uint16_t numberOfSections = fh.NumberOfSections;
    if (numberOfSections == 0 || numberOfSections > 96) return false;

    if (fh.Machine != IMAGE_FILE_MACHINE_AMD64) return false;

    if (fh.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64) || fh.SizeOfOptionalHeader > 0x1000u)
    {
        return false;
    }

    std::uint16_t optMagic = 0;
    std::uintptr_t optMagicAddr = moduleBase + (std::uintptr_t)dos.e_lfanew + 4u + sizeof(IMAGE_FILE_HEADER);
    if (!ReadValue(mem, optMagicAddr, optMagic)) return false;

    if (optMagic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;

    IMAGE_OPTIONAL_HEADER64 opt{};
    if (!ReadValue(mem, optMagicAddr, opt)) return false;
    outSizeOfImage = opt.SizeOfImage;

    if (outSizeOfImage < 0x1000 || outSizeOfImage > 0x40000000) return false;

    std::uintptr_t sectionTable = moduleBase +
        (std::uintptr_t)dos.e_lfanew + 4u +
        sizeof(IMAGE_FILE_HEADER) +
        fh.SizeOfOptionalHeader;

    outSections.reserve(numberOfSections);
    for (std::uint16_t i = 0; i < numberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER sh{};
        if (!ReadValue(mem, sectionTable + (std::uintptr_t)i * sizeof(IMAGE_SECTION_HEADER), sh))
        {
            outSizeOfImage = 0;
            outSections.clear();
            return false;
        }

        ModuleSection ms{};
        std::memset(ms.name, 0, sizeof(ms.name));
        std::memcpy(ms.name, sh.Name, 8);
        ms.name[8] = '\0';
        ms.rva = sh.VirtualAddress;
        ms.size = sh.Misc.VirtualSize;
        if (ms.size == 0) ms.size = sh.SizeOfRawData;
        outSections.push_back(ms);
    }

    return true;
}

inline bool ReadModuleImageBase(const IMemoryAccessor& mem,
                                std::uintptr_t moduleBase,
                                std::uint64_t& outImageBase)
{
    outImageBase = 0;

    IMAGE_DOS_HEADER dos{};
    if (!ReadValue(mem, moduleBase, dos)) return false;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) return false;
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x2000) return false;

    std::uint32_t ntBase = (std::uint32_t)dos.e_lfanew;

    DWORD sig = 0;
    if (!ReadValue(mem, moduleBase + (std::uintptr_t)ntBase, sig)) return false;
    if (sig != IMAGE_NT_SIGNATURE) return false;

    IMAGE_FILE_HEADER fh{};
    if (!ReadValue(mem, moduleBase + (std::uintptr_t)ntBase + 4u, fh)) return false;
    if (fh.Machine != IMAGE_FILE_MACHINE_AMD64) return false;
    if (fh.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64) || fh.SizeOfOptionalHeader > 0x1000u) return false;

    std::uint16_t optMagic = 0;
    std::uintptr_t optMagicAddr = moduleBase + (std::uintptr_t)ntBase + 4u + sizeof(IMAGE_FILE_HEADER);
    if (!ReadValue(mem, optMagicAddr, optMagic)) return false;

    if (optMagic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;

    IMAGE_NT_HEADERS64 nt{};
    if (!ReadValue(mem, moduleBase + (std::uintptr_t)ntBase, nt)) return false;
    if (nt.Signature != IMAGE_NT_SIGNATURE) return false;
    outImageBase = (std::uint64_t)nt.OptionalHeader.ImageBase;
    return true;
}

}

}
