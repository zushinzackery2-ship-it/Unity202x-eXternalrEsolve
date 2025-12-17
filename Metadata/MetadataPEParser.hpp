#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "../Core/UnityExternalMemory.hpp"

namespace UnityExternal {

namespace detail_metadata {

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

    IMAGE_NT_HEADERS64 nt{};
    if (!ReadValue(mem, moduleBase + (std::uintptr_t)dos.e_lfanew, nt)) return false;
    if (nt.Signature != IMAGE_NT_SIGNATURE) return false;
    if (nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return false;
    }

    outSizeOfImage = nt.OptionalHeader.SizeOfImage;
    if (outSizeOfImage < 0x1000 || outSizeOfImage > 0x40000000) return false;

    std::uint16_t numberOfSections = nt.FileHeader.NumberOfSections;
    if (numberOfSections == 0 || numberOfSections > 96) return false;

    std::uintptr_t sectionTable = moduleBase +
        (std::uintptr_t)dos.e_lfanew + 4u +
        sizeof(IMAGE_FILE_HEADER) +
        nt.FileHeader.SizeOfOptionalHeader;

    outSections.reserve(numberOfSections);
    for (std::uint16_t i = 0; i < numberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER sh{};
        if (!ReadValue(mem, sectionTable + (std::uintptr_t)i * sizeof(IMAGE_SECTION_HEADER), sh))
        {
            break;
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

    return !outSections.empty();
}

}

}

#endif
