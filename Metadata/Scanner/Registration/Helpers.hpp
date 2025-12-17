#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "../../../Core/UnityExternalMemory.hpp"
#include "Types.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

inline bool EqualsIgnoreCase(const char* a, const char* b)
{
    if (!a || !b) return false;
    while (*a && *b)
    {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return false;
        ++a;
        ++b;
    }
    return *a == 0 && *b == 0;
}

inline bool GetDiskPeSections(const std::filesystem::path& pePath, std::vector<DiskSection>& out)
{
    out.clear();

    std::ifstream ifs(pePath, std::ios::binary | std::ios::in);
    if (!ifs.good()) return false;

    IMAGE_DOS_HEADER dos{};
    ifs.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!ifs.good()) return false;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) return false;
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x4000) return false;

    ifs.seekg((std::streamoff)dos.e_lfanew, std::ios::beg);
    if (!ifs.good()) return false;

    DWORD sig = 0;
    ifs.read(reinterpret_cast<char*>(&sig), sizeof(sig));
    if (!ifs.good()) return false;
    if (sig != IMAGE_NT_SIGNATURE) return false;

    IMAGE_FILE_HEADER fh{};
    ifs.read(reinterpret_cast<char*>(&fh), sizeof(fh));
    if (!ifs.good()) return false;

    if (fh.NumberOfSections == 0 || fh.NumberOfSections > 128) return false;

    std::streamoff sectionTableOff = (std::streamoff)dos.e_lfanew + 4 + (std::streamoff)sizeof(IMAGE_FILE_HEADER) + (std::streamoff)fh.SizeOfOptionalHeader;
    ifs.seekg(sectionTableOff, std::ios::beg);
    if (!ifs.good()) return false;

    out.reserve(fh.NumberOfSections);
    for (std::uint16_t i = 0; i < fh.NumberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER sh{};
        ifs.read(reinterpret_cast<char*>(&sh), sizeof(sh));
        if (!ifs.good()) break;

        DiskSection s{};
        std::memset(s.name, 0, sizeof(s.name));
        std::memcpy(s.name, sh.Name, 8);
        s.name[8] = '\0';
        s.rva = sh.VirtualAddress;
        s.vsize = sh.Misc.VirtualSize;
        if (s.vsize == 0) s.vsize = sh.SizeOfRawData;
        out.push_back(s);
    }

    return !out.empty();
}

inline bool InAny(std::uintptr_t addr, const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& ranges)
{
    for (const auto& r : ranges)
    {
        if (addr >= r.first && addr < r.second) return true;
    }
    return false;
}

inline void BuildRanges(std::uintptr_t moduleBase,
                        const std::vector<DiskSection>& secs,
                        std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
                        std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& dataRanges,
                        std::vector<DiskSection>& dataSecs)
{
    execRanges.clear();
    dataRanges.clear();
    dataSecs.clear();

    for (const auto& s : secs)
    {
        if (s.vsize == 0) continue;

        std::uintptr_t start = moduleBase + (std::uintptr_t)s.rva;
        std::uintptr_t end = start + (std::uintptr_t)s.vsize;

        if (EqualsIgnoreCase(s.name, ".text"))
        {
            execRanges.push_back(std::make_pair(start, end));
        }
        else if (EqualsIgnoreCase(s.name, ".data") || EqualsIgnoreCase(s.name, ".rdata") || EqualsIgnoreCase(s.name, ".pdata") ||
                 EqualsIgnoreCase(s.name, ".tls") || EqualsIgnoreCase(s.name, ".reloc"))
        {
            dataRanges.push_back(std::make_pair(start, end));
        }

        if (EqualsIgnoreCase(s.name, ".data") || EqualsIgnoreCase(s.name, ".rdata"))
        {
            dataSecs.push_back(s);
        }
    }

    if (dataSecs.empty())
    {
        dataSecs = secs;
    }
}

inline bool ContainsDllCaseInsensitive(const std::string& s)
{
    for (std::size_t i = 0; i + 3 < s.size(); ++i)
    {
        char c0 = s[i + 0];
        char c1 = s[i + 1];
        char c2 = s[i + 2];
        char c3 = s[i + 3];
        if (c0 >= 'A' && c0 <= 'Z') c0 = (char)(c0 - 'A' + 'a');
        if (c1 >= 'A' && c1 <= 'Z') c1 = (char)(c1 - 'A' + 'a');
        if (c2 >= 'A' && c2 <= 'Z') c2 = (char)(c2 - 'A' + 'a');
        if (c3 >= 'A' && c3 <= 'Z') c3 = (char)(c3 - 'A' + 'a');
        if (c0 == '.' && c1 == 'd' && c2 == 'l' && c3 == 'l') return true;
    }
    return false;
}

inline std::uint64_t U64At(const std::vector<std::uint8_t>& buf, std::size_t off)
{
    std::uint64_t v = 0;
    if (off + sizeof(v) <= buf.size()) std::memcpy(&v, buf.data() + off, sizeof(v));
    return v;
}

inline std::int64_t I64At(const std::vector<std::uint8_t>& buf, std::size_t off)
{
    std::int64_t v = 0;
    if (off + sizeof(v) <= buf.size()) std::memcpy(&v, buf.data() + off, sizeof(v));
    return v;
}

inline bool ReadChunk(const IMemoryAccessor& mem, std::uintptr_t addr, std::size_t size, std::vector<std::uint8_t>& out)
{
    out.clear();
    if (size == 0) return false;
    out.resize(size);
    if (!mem.Read(addr, out.data(), size))
    {
        out.clear();
        return false;
    }
    return true;
}

inline bool CheckCodeGenModulesArray(const IMemoryAccessor& mem,
                                    std::uintptr_t moduleBase,
                                    std::uintptr_t moduleEnd,
                                    std::uintptr_t codeGenModules,
                                    int sample)
{
    if (codeGenModules == 0) return false;
    if (codeGenModules < moduleBase || codeGenModules >= moduleEnd) return false;

    for (int i = 0; i < sample; ++i)
    {
        std::uintptr_t pmod = 0;
        if (!ReadPtr(mem, codeGenModules + (std::uintptr_t)i * 8u, pmod)) return false;
        if (pmod == 0 || pmod < moduleBase || pmod >= moduleEnd) return false;

        std::uintptr_t moduleNamePtr = 0;
        if (!ReadPtr(mem, pmod + 0u, moduleNamePtr)) return false;

        std::string s;
        if (!ReadCString(mem, moduleNamePtr, s, 260)) return false;
        if (s.empty()) return false;
        if (!ContainsDllCaseInsensitive(s)) return false;
    }

    return true;
}

inline bool CheckPointerArrayPointsIntoExec(const IMemoryAccessor& mem,
                                           std::uintptr_t ptr,
                                           std::uintptr_t moduleBase,
                                           std::uintptr_t moduleEnd,
                                           const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
                                           int sample)
{
    if (ptr == 0 || ptr < moduleBase || ptr >= moduleEnd) return false;

    for (int i = 0; i < sample; ++i)
    {
        std::uintptr_t p = 0;
        if (!ReadPtr(mem, ptr + (std::uintptr_t)i * 8u, p)) return false;
        if (p == 0) return false;
        if (!InAny(p, execRanges)) return false;
    }

    return true;
}

inline std::vector<std::int64_t> InferTypeDefCounts(std::uint32_t typeDefSize)
{
    static const std::uint32_t candidates[] = {
        0x58, 0x54, 0x50, 0x4C, 0x48, 0x44, 0x40, 0x3C, 0x38, 0x34, 0x30, 0x2C, 0x28, 0x24, 0x20,
    };

    std::vector<std::int64_t> out;
    if (typeDefSize == 0) return out;

    for (std::uint32_t sz : candidates)
    {
        if ((typeDefSize % sz) != 0) continue;
        std::uint32_t cnt = typeDefSize / sz;
        if (cnt == 0 || cnt > 300000u) continue;
        out.push_back((std::int64_t)cnt);
    }
    return out;
}

inline bool HasCountCandidate(const std::vector<std::int64_t>& candidates, std::int64_t value)
{
    for (std::int64_t c : candidates)
    {
        if (c == value) return true;
    }
    return false;
}

}

}

#endif
