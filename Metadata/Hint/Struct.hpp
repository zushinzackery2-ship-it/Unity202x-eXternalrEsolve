#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace UnityExternal
{

struct MetadataHint
{
    std::uint32_t schema = 1;

    DWORD pid = 0;
    std::wstring processName;

    std::wstring moduleName;
    std::wstring modulePath;
    std::uintptr_t moduleBase = 0;
    std::uint32_t moduleSize = 0;
    std::uint64_t peImageBase = 0;

    std::uintptr_t sGlobalMetadataAddr = 0;
    std::uintptr_t metaBase = 0;
    std::uint32_t totalSize = 0;
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t imagesCount = 0;
    std::uint32_t assembliesCount = 0;

    std::uintptr_t codeRegistration = 0;
    std::uintptr_t metadataRegistration = 0;
    std::uint64_t codeRegistrationRva = 0;
    std::uint64_t metadataRegistrationRva = 0;
};

namespace detail_metadata
{

inline bool TryQueryModuleEntry(DWORD pid, const std::wstring& moduleName, MODULEENTRY32W& outMe)
{
    std::memset(&outMe, 0, sizeof(outMe));
    outMe.dwSize = sizeof(outMe);
    if (!pid || moduleName.empty()) return false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    bool ok = false;
    if (Module32FirstW(snapshot, &outMe))
    {
        do
        {
            if (_wcsicmp(outMe.szModule, moduleName.c_str()) == 0)
            {
                ok = true;
                break;
            }
        } while (Module32NextW(snapshot, &outMe));
    }
    CloseHandle(snapshot);
    return ok;
}

inline bool TryReadPeImageBaseFromFile(const std::filesystem::path& modulePath, std::uint64_t& outImageBase)
{
    outImageBase = 0;

    std::ifstream ifs(modulePath, std::ios::binary | std::ios::in);
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

    std::uint16_t optMagic = 0;
    ifs.read(reinterpret_cast<char*>(&optMagic), sizeof(optMagic));
    if (!ifs.good()) return false;
    ifs.seekg(-2, std::ios::cur);
    if (!ifs.good()) return false;

    if (optMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        IMAGE_OPTIONAL_HEADER64 opt{};
        ifs.read(reinterpret_cast<char*>(&opt), sizeof(opt));
        if (!ifs.good()) return false;
        outImageBase = (std::uint64_t)opt.ImageBase;
        return outImageBase != 0;
    }

    if (optMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        IMAGE_OPTIONAL_HEADER32 opt{};
        ifs.read(reinterpret_cast<char*>(&opt), sizeof(opt));
        if (!ifs.good()) return false;
        outImageBase = (std::uint64_t)opt.ImageBase;
        return outImageBase != 0;
    }

    return false;
}

}

}

#endif
