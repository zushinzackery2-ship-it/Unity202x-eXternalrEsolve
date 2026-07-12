#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../os/win/win_memory_accessor.hpp"
#include "../../os/win/win_module.hpp"
#include "../../os/win/win_path.hpp"
#include "../../os/win/win_process.hpp"

#include "../metadata.hpp"
#include "../metadata/metadata_images.hpp"

#include "sdk_common.hpp"
#include "sdk_dump_cs.hpp"
#include "sdk_generic_json.hpp"
#include "sdk_metadata_helpers.hpp"
#include "sdk_registration.hpp"
#include "sdk_strings.hpp"

namespace er2
{

struct DumpSdk6Paths
{
    std::string outDir;
    std::string dumpCsPath;
    std::string genericJsonPath;
};

namespace detail_dumpsdk
{

inline void SetError(std::string* error, const char* message)
{
    if (error != nullptr)
    {
        *error = message != nullptr ? message : "";
    }
}

inline bool WriteMetadataFile(const std::string& outDir, const std::vector<std::uint8_t>& metaBytes, std::string* error)
{
    const std::string path = JoinPathA(outDir, "global-metadata.dat");
    std::ofstream out(path, std::ios::binary);
    if (!out)
    {
        SetError(error, "failed to write global-metadata.dat");
        return false;
    }

    if (!metaBytes.empty())
    {
        out.write(reinterpret_cast<const char*>(metaBytes.data()),
            static_cast<std::streamsize>(metaBytes.size()));
        if (!out)
        {
            SetError(error, "failed to write global-metadata.dat bytes");
            return false;
        }
    }

    return true;
}

inline bool WriteHintJson(const std::string& outDir, const MetadataHint& hint, std::string* error)
{
    if (hint.metadataRegistration == 0 && hint.codeRegistration == 0
        && hint.metadataRegistrationRva == 0 && hint.codeRegistrationRva == 0)
    {
        return true;
    }

    const std::string path = JoinPathA(outDir, "il2cpp-offline.hint.json");
    std::ofstream out(path);
    if (!out)
    {
        SetError(error, "failed to write il2cpp-offline.hint.json");
        return false;
    }

    const std::uintptr_t moduleBase = hint.moduleBase;
    const std::uint64_t codeRva = hint.codeRegistrationRva != 0
        ? hint.codeRegistrationRva
        : ((hint.codeRegistration >= moduleBase && moduleBase != 0)
            ? static_cast<std::uint64_t>(hint.codeRegistration - moduleBase)
            : 0);
    const std::uint64_t metaRva = hint.metadataRegistrationRva != 0
        ? hint.metadataRegistrationRva
        : ((hint.metadataRegistration >= moduleBase && moduleBase != 0)
            ? static_cast<std::uint64_t>(hint.metadataRegistration - moduleBase)
            : 0);

    out << "{\n";
    out << "  \"module\": {\n";
    out << "    \"base_addr\": \"0x" << std::hex << moduleBase << "\",\n";
    out << "    \"code_registration_rva\": \"0x" << codeRva << "\",\n";
    out << "    \"metadata_registration_rva\": \"0x" << metaRva << "\"\n";
    out << "  }\n";
    out << "}\n";

    if (!out)
    {
        SetError(error, "failed to flush il2cpp-offline.hint.json");
        return false;
    }

    return true;
}

} // namespace detail_dumpsdk

inline bool DumpSdkDump(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const std::string& outDir,
    DumpSdk6Paths& outPaths,
    std::string* error = nullptr,
    bool writeDumpCs = true)
{
    (void)moduleSize;

    outPaths = DumpSdk6Paths{};
    if (error != nullptr)
    {
        error->clear();
    }

    if (moduleBase == 0)
    {
        detail_dumpsdk::SetError(error, "moduleBase is null");
        return false;
    }

    if (outDir.empty())
    {
        detail_dumpsdk::SetError(error, "outDir is empty");
        return false;
    }

    MetadataHint hint;
    if (!BuildMetadataHintTScore(
            mem,
            moduleBase,
            GetCurrentProcessId(),
            L"",
            L"GameAssembly.dll",
            hint))
    {
        detail_dumpsdk::SetError(error, "BuildMetadataHintTScore failed");
        return false;
    }

    std::vector<std::uint8_t> metaBytes;
    if (!ExportMetadataByScore(mem, moduleBase, 0x200000u, 8192, 15.0, false, 0, 0x200000u, metaBytes))
    {
        detail_dumpsdk::SetError(error, "ExportMetadataByScore failed");
        return false;
    }

    MetadataHeaderFields header;
    if (!ReadMetadataHeaderFieldsFromBytes(metaBytes, header))
    {
        detail_dumpsdk::SetError(error, "ReadMetadataHeaderFieldsFromBytes failed");
        return false;
    }

    std::vector<MetadataImageInfo> images;
    (void)ReadImagesFromBytes(metaBytes, header, images);

    std::vector<std::string> typeToImage;
    (void)BuildTypeDefIndexToImageNameFromBytes(metaBytes, typeToImage);

    std::unordered_map<std::uint32_t, std::string> typeMap;
    std::vector<std::string> typeFullName;
    (void)BuildTypeFullNameAndByvalMapFromBytes(metaBytes, header, typeFullName, typeMap);

    std::vector<DumpSdk6GenericParamInfo> genericParams = BuildGenericParamInfoFromBytes(metaBytes, header);

    std::uintptr_t typesPtr = 0;
    std::uint32_t typesCount = 0;
    std::uintptr_t fieldOffsetsPtr = 0;
    (void)DumpSdk6GetMetadataRegistrationTypes(
        mem,
        hint.metadataRegistration,
        header.version,
        typesPtr,
        typesCount,
        fieldOffsetsPtr);

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(outDir), ec);

    if (!detail_dumpsdk::WriteMetadataFile(outDir, metaBytes, error))
    {
        return false;
    }

    if (!detail_dumpsdk::WriteHintJson(outDir, hint, error))
    {
        return false;
    }

    const std::string dumpCsPath = JoinPathA(outDir, "dump.cs");
    const std::string genericJsonPath = JoinPathA(outDir, "generic.json");

    if (!DumpSdk6WriteGenericJsonFile(genericJsonPath, header, metaBytes, genericParams, typeFullName))
    {
        detail_dumpsdk::SetError(error, "DumpSdk6WriteGenericJsonFile failed");
        return false;
    }

    if (writeDumpCs)
    {
        if (!DumpSdk6WriteDumpCsFile(
                dumpCsPath,
                mem,
                hint,
                header,
                metaBytes,
                images,
                typeToImage,
                typeFullName,
                typeMap,
                genericParams,
                typesPtr,
                typesCount,
                fieldOffsetsPtr))
        {
            detail_dumpsdk::SetError(error, "DumpSdk6WriteDumpCsFile failed");
            return false;
        }
    }

    outPaths.outDir = outDir;
    outPaths.dumpCsPath = writeDumpCs ? dumpCsPath : std::string{};
    outPaths.genericJsonPath = genericJsonPath;
    return true;
}

inline bool DumpSdk6DumpByPid(std::uint32_t pid, DumpSdk6Paths& outPaths)
{
    outPaths = DumpSdk6Paths{};

    HANDLE hProc = OpenProcessForRead(pid);
    if (!hProc)
    {
        return false;
    }

    WinApiMemoryAccessor mem(hProc);

    ModuleInfo gameAssembly;
    if (!GetRemoteModuleInfo(pid, L"GameAssembly.dll", gameAssembly) || !gameAssembly.base || gameAssembly.size == 0)
    {
        CloseHandle(hProc);
        return false;
    }

    const std::string outDir = JoinPathA(GetExeDirA(), "DumpSDK");
    const bool ok = DumpSdkDump(mem, gameAssembly.base, gameAssembly.size, outDir, outPaths, nullptr);
    CloseHandle(hProc);
    return ok;
}

} // namespace er2
