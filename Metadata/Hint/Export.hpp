#pragma once

#include <filesystem>
#include <tlhelp32.h>
#include <fstream>
#include <string>
#include <vector>

#include "../../Core/UnityExternalMemoryConfig.hpp"
#include "../Core/Config.hpp"
#include "../PE/Parser.hpp"
#include "../Header/Parser.hpp"
#include "../Scanner/Pointer.hpp"
#include "../Scanner/Registration/Scanner.hpp"
#include "Struct.hpp"
#include "Json.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

inline bool TryQueryModulePath(DWORD pid, const wchar_t* moduleName, std::wstring& outPath)
{
    outPath.clear();
    if (!pid) return false;
    if (!moduleName || !moduleName[0]) return false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);

    bool ok = false;
    if (Module32FirstW(snapshot, &me))
    {
        do
        {
            if (_wcsicmp(me.szModule, moduleName) == 0)
            {
                outPath = me.szExePath;
                ok = !outPath.empty();
                break;
            }
        } while (Module32NextW(snapshot, &me));
    }

    CloseHandle(snapshot);
    return ok;
}

inline bool BuildMetadataHintImpl(std::uint32_t requiredVersion,
                                 bool strictVersion,
                                 DWORD pid,
                                 const wchar_t* processName,
                                 const wchar_t* moduleName,
                                 MetadataHint& outHint)
{
    outHint = MetadataHint{};
    outHint.pid = pid;
    outHint.processName = processName ? processName : L"";
    outHint.moduleName = moduleName ? moduleName : L"";
    outHint.modulePath.clear();

    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return false;

    std::uintptr_t moduleBase = GetMetadataTargetModule();
    if (!moduleBase) return false;

    outHint.moduleBase = moduleBase;

    if (pid && moduleName && moduleName[0])
    {
        std::wstring modulePath;
        if (TryQueryModulePath(pid, moduleName, modulePath))
        {
            outHint.modulePath = modulePath;
        }
    }

    std::uint32_t sizeOfImage = 0;
    std::vector<ModuleSection> sections;
    if (!ReadModuleSections(*acc, moduleBase, sizeOfImage, sections))
    {
        return false;
    }

    outHint.moduleSize = sizeOfImage;

    std::uint64_t imageBase = 0;
    if (ReadModuleImageBase(*acc, moduleBase, imageBase))
    {
        outHint.peImageBase = imageBase;
    }

    FoundMeta found = FindMetadataPointerByScore(
        *acc,
        moduleBase,
        sizeOfImage,
        0x200000u,
        8192,
        15.0,
        strictVersion,
        requiredVersion);

    if (!found.metaBase) return false;

    outHint.sGlobalMetadataAddr = found.ptrAddr;
    outHint.metaBase = found.metaBase;

    std::uint32_t totalSize = 0;
    if (!CalcTotalSizeFromHeader(*acc, found.metaBase, totalSize)) return false;

    outHint.totalSize = totalSize;
    (void)ReadRemoteU32(*acc, found.metaBase + 0x00u, outHint.magic);
    (void)ReadRemoteU32(*acc, found.metaBase + 0x04u, outHint.version);

    std::uint32_t imagesSize = 0;
    std::uint32_t assembliesSize = 0;
    (void)ReadRemoteU32(*acc, found.metaBase + 0xACu, imagesSize);
    (void)ReadRemoteU32(*acc, found.metaBase + 0xB4u, assembliesSize);
    if (imagesSize > 0 && (imagesSize % 0x28u) == 0u)
    {
        outHint.imagesCount = imagesSize / 0x28u;
    }
    if (assembliesSize > 0 && (assembliesSize % 0x40u) == 0u)
    {
        outHint.assembliesCount = assembliesSize / 0x40u;
    }

    if (!outHint.modulePath.empty() && outHint.moduleBase && outHint.moduleSize)
    {
        Il2CppRegs regs{};
        if (FindIl2CppRegistrations(*acc,
                                    outHint.moduleBase,
                                    outHint.moduleSize,
                                    std::filesystem::path(outHint.modulePath),
                                    found.metaBase,
                                    0x20000u,
                                    10.0,
                                    regs))
        {
            outHint.codeRegistration = regs.codeRegistration;
            outHint.metadataRegistration = regs.metadataRegistration;

            if (regs.codeRegistration && regs.codeRegistration >= outHint.moduleBase)
            {
                outHint.codeRegistrationRva = (std::uint64_t)(regs.codeRegistration - outHint.moduleBase);
            }
            if (regs.metadataRegistration && regs.metadataRegistration >= outHint.moduleBase)
            {
                outHint.metadataRegistrationRva = (std::uint64_t)(regs.metadataRegistration - outHint.moduleBase);
            }
        }
    }

    return true;
}

}

inline std::string ExportMetadataHintJsonTScore(DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)
{
    MetadataHint hint;
    if (!detail_metadata::BuildMetadataHintImpl(0, false, pid, processName, moduleName, hint))
    {
        return std::string();
    }
    return BuildMetadataHintJson(hint);
}

inline std::string ExportMetadataHintJsonTVersion(DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)
{
    std::uint32_t version = GetMetadataTargetVersion();
    MetadataHint hint;
    if (version == 0)
    {
        if (!detail_metadata::BuildMetadataHintImpl(0, true, pid, processName, moduleName, hint))
        {
            return std::string();
        }
        return BuildMetadataHintJson(hint);
    }
    if (!detail_metadata::BuildMetadataHintImpl(version, false, pid, processName, moduleName, hint))
    {
        return std::string();
    }
    return BuildMetadataHintJson(hint);
}

inline bool ExportMetadataHintJsonTScoreToFile(const std::filesystem::path& outPath,
                                              DWORD pid = 0,
                                              const wchar_t* processName = nullptr,
                                              const wchar_t* moduleName = nullptr)
{
    std::string json = ExportMetadataHintJsonTScore(pid, processName, moduleName);
    if (json.empty()) return false;

    std::ofstream ofs(outPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.good()) return false;
    ofs.write(json.data(), (std::streamsize)json.size());
    return ofs.good();
}

inline bool ExportMetadataHintJsonTVersionToFile(const std::filesystem::path& outPath,
                                                DWORD pid = 0,
                                                const wchar_t* processName = nullptr,
                                                const wchar_t* moduleName = nullptr)
{
    std::string json = ExportMetadataHintJsonTVersion(pid, processName, moduleName);
    if (json.empty()) return false;

    std::ofstream ofs(outPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.good()) return false;
    ofs.write(json.data(), (std::streamsize)json.size());
    return ofs.good();
}

inline bool ExportMetadataHintJsonTScoreToSidecar(const std::filesystem::path& metadataDatPath,
                                                 DWORD pid = 0,
                                                 const wchar_t* processName = nullptr,
                                                 const wchar_t* moduleName = nullptr)
{
    std::filesystem::path out = metadataDatPath;
    out += L".hint.json";
    return ExportMetadataHintJsonTScoreToFile(out, pid, processName, moduleName);
}

}
