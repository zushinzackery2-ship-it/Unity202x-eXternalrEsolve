#pragma once

#include <TlHelp32.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "codegen_modules.hpp"
#include "header_parser.hpp"
#include "hint_json.hpp"
#include "pe.hpp"
#include "registration_scanner.hpp"
#include "scanner_pointer.hpp"

namespace er2
{

namespace detail_metadata_hint
{

inline bool TryQueryModulePath(std::uint32_t pid, const wchar_t* moduleName, std::wstring& outPath)
{
    outPath.clear();
    if (!pid)
    {
        return false;
    }
    if (!moduleName || !moduleName[0])
    {
        return false;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<DWORD>(pid));
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    MODULEENTRY32W me;
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


inline bool BuildMetadataHintImpl(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::size_t scanChunkSize,
    std::size_t maxPages,
    double maxSeconds,
    std::uint32_t requiredVersion,
    bool strictVersion,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName,
    MetadataHint& outHint)
{
    outHint = MetadataHint{};

    outHint.pid = pid;
    outHint.processName = processName ? processName : L"";
    outHint.moduleName = moduleName ? moduleName : L"";

    outHint.moduleBase = moduleBase;
    if (!moduleBase)
    {
        return false;
    }

    if (pid && moduleName && moduleName[0])
    {
        std::wstring modulePath;
        if (TryQueryModulePath(pid, moduleName, modulePath))
        {
            outHint.modulePath = modulePath;
        }
    }

    std::vector<ModuleSection> sections;
    std::uint32_t sizeOfImage = 0;
    if (!ReadModuleSections(mem, moduleBase, sizeOfImage, sections))
    {
        return false;
    }

    outHint.moduleSize = sizeOfImage;

    std::uint64_t imageBase = 0;
    if (ReadModuleImageBase(mem, moduleBase, imageBase))
    {
        outHint.peImageBase = imageBase;
    }

    const FoundMetadata found = FindMetadataPointerByScore(mem, moduleBase, scanChunkSize, maxPages, maxSeconds, strictVersion, requiredVersion);
    if (!found.metaBase)
    {
        return false;
    }

    outHint.sGlobalMetadataAddr = found.ptrAddr;
    outHint.metaBase = found.metaBase;

    std::uint32_t totalSize = 0;
    if (!CalcTotalSizeFromHeader(mem, found.metaBase, totalSize))
    {
        return false;
    }
    outHint.totalSize = totalSize;

    (void)ReadValue(mem, found.metaBase + 0x00u, outHint.magic);
    (void)ReadValue(mem, found.metaBase + 0x04u, outHint.version);

    std::uint32_t imagesSize = 0;
    std::uint32_t assembliesSize = 0;
    (void)ReadValue(mem, found.metaBase + 0xACu, imagesSize);
    (void)ReadValue(mem, found.metaBase + 0xB4u, assembliesSize);

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
        Il2CppRegs regs;
        if (FindIl2CppRegistrations(
                mem,
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
                outHint.codeRegistrationRva = static_cast<std::uint64_t>(regs.codeRegistration - outHint.moduleBase);
            }
            if (regs.metadataRegistration && regs.metadataRegistration >= outHint.moduleBase)
            {
                outHint.metadataRegistrationRva = static_cast<std::uint64_t>(regs.metadataRegistration - outHint.moduleBase);
            }
        }
    }

    // Optional: export CodeGenModules (used by runtime RVA/token mapping).
    {
        std::uintptr_t cgArray = 0;
        std::uint32_t cgCount = 0;
        std::vector<CodeGenModuleHint> cgList;

        if (CollectCodeGenModules(
                mem,
                outHint.moduleBase,
                outHint.moduleSize,
                sections,
                outHint.version,
                outHint.imagesCount,
                outHint.codeRegistration,
                cgArray,
                cgCount,
                cgList))
        {
            outHint.codeGenModules = cgArray;
            outHint.codeGenModulesCount = cgCount;
            outHint.codeGenModuleList = std::move(cgList);
        }
    }

    return true;
}

}

inline bool BuildMetadataHintTScore(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName,
    MetadataHint& outHint,
    std::size_t scanChunkSize = 0x200000u,
    std::size_t maxPages = 8192,
    double maxSeconds = 15.0)
{
    return detail_metadata_hint::BuildMetadataHintImpl(mem, moduleBase, scanChunkSize, maxPages, maxSeconds, 0, false, pid, processName, moduleName, outHint);
}

inline bool BuildMetadataHintTVersion(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t requiredVersion,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName,
    MetadataHint& outHint,
    std::size_t scanChunkSize = 0x200000u,
    std::size_t maxPages = 8192,
    double maxSeconds = 15.0)
{
    const bool strictVersion = (requiredVersion == 0);
    return detail_metadata_hint::BuildMetadataHintImpl(mem, moduleBase, scanChunkSize, maxPages, maxSeconds, requiredVersion, strictVersion, pid, processName, moduleName, outHint);
}

inline bool ExportMetadataHintJsonTScore(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName,
    std::string& outJson,
    std::size_t scanChunkSize = 0x200000u,
    std::size_t maxPages = 8192,
    double maxSeconds = 15.0)
{
    outJson.clear();

    MetadataHint hint;
    if (!BuildMetadataHintTScore(mem, moduleBase, pid, processName, moduleName, hint, scanChunkSize, maxPages, maxSeconds))
    {
        return false;
    }

    outJson = BuildMetadataHintJson(hint);
    return !outJson.empty();
}

inline bool ExportMetadataHintJsonTVersion(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t requiredVersion,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName,
    std::string& outJson,
    std::size_t scanChunkSize = 0x200000u,
    std::size_t maxPages = 8192,
    double maxSeconds = 15.0)
{
    outJson.clear();

    MetadataHint hint;
    if (!BuildMetadataHintTVersion(mem, moduleBase, requiredVersion, pid, processName, moduleName, hint, scanChunkSize, maxPages, maxSeconds))
    {
        return false;
    }

    outJson = BuildMetadataHintJson(hint);
    return !outJson.empty();
}

inline bool ExportMetadataHintJsonTScoreToFile(
    const IMemoryAccessor& mem,
    const std::filesystem::path& outPath,
    std::uintptr_t moduleBase,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName)
{
    std::string json;
    if (!ExportMetadataHintJsonTScore(mem, moduleBase, pid, processName, moduleName, json))
    {
        return false;
    }

    std::ofstream ofs(outPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.good())
    {
        return false;
    }

    ofs.write(json.data(), static_cast<std::streamsize>(json.size()));
    return ofs.good();
}

inline bool ExportMetadataHintJsonTVersionToFile(
    const IMemoryAccessor& mem,
    const std::filesystem::path& outPath,
    std::uintptr_t moduleBase,
    std::uint32_t requiredVersion,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName)
{
    std::string json;
    if (!ExportMetadataHintJsonTVersion(mem, moduleBase, requiredVersion, pid, processName, moduleName, json))
    {
        return false;
    }

    std::ofstream ofs(outPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.good())
    {
        return false;
    }

    ofs.write(json.data(), static_cast<std::streamsize>(json.size()));
    return ofs.good();
}

inline bool ExportMetadataHintJsonTScoreToSidecar(
    const IMemoryAccessor& mem,
    const std::filesystem::path& metadataDatPath,
    std::uintptr_t moduleBase,
    std::uint32_t pid,
    const wchar_t* processName,
    const wchar_t* moduleName)
{
    std::filesystem::path out = metadataDatPath;
    out += L".hint.json";
    return ExportMetadataHintJsonTScoreToFile(mem, out, moduleBase, pid, processName, moduleName);
}

}
