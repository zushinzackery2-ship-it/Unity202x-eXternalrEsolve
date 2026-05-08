#pragma once

#include "../../../os/win/win_include.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

#include "../../../mem/memory_read.hpp"
#include "registration_types.hpp"
#include "string_utils.hpp"

namespace er2
{

namespace detail_il2cpp_reg
{

inline bool GetDiskPeSections(const std::filesystem::path& pePath, std::vector<DiskSection>& out)
{
    out.clear();

    std::ifstream ifs(pePath, std::ios::binary | std::ios::in);
    if (!ifs.good())
    {
        return false;
    }

    IMAGE_DOS_HEADER dos;
    ifs.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!ifs.good())
    {
        return false;
    }
    if (dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x4000)
    {
        return false;
    }

    ifs.seekg(static_cast<std::streamoff>(dos.e_lfanew), std::ios::beg);
    if (!ifs.good())
    {
        return false;
    }

    DWORD sig = 0;
    ifs.read(reinterpret_cast<char*>(&sig), sizeof(sig));
    if (!ifs.good())
    {
        return false;
    }
    if (sig != IMAGE_NT_SIGNATURE)
    {
        return false;
    }

    IMAGE_FILE_HEADER fh;
    ifs.read(reinterpret_cast<char*>(&fh), sizeof(fh));
    if (!ifs.good())
    {
        return false;
    }

    if (fh.Machine != IMAGE_FILE_MACHINE_AMD64)
    {
        return false;
    }
    if (fh.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64) || fh.SizeOfOptionalHeader > 0x1000u)
    {
        return false;
    }

    std::uint16_t optMagic = 0;
    ifs.read(reinterpret_cast<char*>(&optMagic), sizeof(optMagic));
    if (!ifs.good())
    {
        return false;
    }
    if (optMagic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return false;
    }

    if (fh.NumberOfSections == 0 || fh.NumberOfSections > 128)
    {
        return false;
    }

    std::streamoff sectionTableOff = static_cast<std::streamoff>(dos.e_lfanew) + 4 + static_cast<std::streamoff>(sizeof(IMAGE_FILE_HEADER)) + static_cast<std::streamoff>(fh.SizeOfOptionalHeader);
    ifs.seekg(sectionTableOff, std::ios::beg);
    if (!ifs.good())
    {
        return false;
    }

    out.reserve(fh.NumberOfSections);
    for (std::uint16_t i = 0; i < fh.NumberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER sh;
        ifs.read(reinterpret_cast<char*>(&sh), sizeof(sh));
        if (!ifs.good())
        {
            break;
        }

        DiskSection s;
        std::memset(s.name, 0, sizeof(s.name));
        std::memcpy(s.name, sh.Name, 8);
        s.name[8] = '\0';
        s.rva = sh.VirtualAddress;
        s.vsize = sh.Misc.VirtualSize;
        if (s.vsize == 0)
        {
            s.vsize = sh.SizeOfRawData;
        }
        s.rawPtr = sh.PointerToRawData;
        s.rawSize = sh.SizeOfRawData;
        out.push_back(s);
    }

    return !out.empty();
}

inline void BuildRanges(
    std::uintptr_t moduleBase,
    const std::vector<DiskSection>& secs,
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& dataRanges,
    std::vector<DiskSection>& dataSecs)
{
    execRanges.clear();
    dataRanges.clear();
    dataSecs.clear();

    // Unity 6000+ GameAssembly may use non-standard section names.
    // - "il2cpp" often contains executable code (method bodies) and pointer tables.
    // - "_RDATA" sometimes appears instead of ".rdata".
    for (const auto& s : secs)
    {
        if (s.vsize == 0)
        {
            continue;
        }

        const std::uintptr_t start = moduleBase + static_cast<std::uintptr_t>(s.rva);
        const std::uintptr_t end = start + static_cast<std::uintptr_t>(s.vsize);

        // Executable ranges (used for validating method pointers / invokers).
        if (EqualsIgnoreCase(s.name, ".text") || EqualsIgnoreCase(s.name, "il2cpp") || EqualsIgnoreCase(s.name, ".il2cpp"))
        {
            execRanges.push_back(std::make_pair(start, end));
        }

        // Data-like ranges (used for validating pointer arrays / tables).
        // Keep this whitelist conservative to avoid false positives.
        if (EqualsIgnoreCase(s.name, ".data") || EqualsIgnoreCase(s.name, ".rdata") || EqualsIgnoreCase(s.name, "_rdata") || EqualsIgnoreCase(s.name, ".pdata") || EqualsIgnoreCase(s.name, ".tls") || EqualsIgnoreCase(s.name, ".reloc") || EqualsIgnoreCase(s.name, "il2cpp"))
        {
            dataRanges.push_back(std::make_pair(start, end));
        }

        // Sections we actually scan for registration candidates.
        // Keep this small for performance; the registrations are expected to live in data sections.
        if (EqualsIgnoreCase(s.name, ".data") || EqualsIgnoreCase(s.name, ".rdata") || EqualsIgnoreCase(s.name, "_rdata"))
        {
            dataSecs.push_back(s);
        }
    }
}

}

}
