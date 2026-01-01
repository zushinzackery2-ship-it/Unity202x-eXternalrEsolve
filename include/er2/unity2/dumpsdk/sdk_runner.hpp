#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../os/win/win_memory_accessor.hpp"
#include "../../os/win/win_module.hpp"
#include "../../os/win/win_process.hpp"

#include "../metadata.hpp"
#include "../metadata/metadata_images.hpp"

#include "path.hpp"
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

    MetadataHint hint;
    if (!BuildMetadataHintTScore(mem, gameAssembly.base, pid, L"", L"GameAssembly.dll", hint))
    {
        CloseHandle(hProc);
        return false;
    }

    std::vector<std::uint8_t> metaBytes;
    if (!ExportMetadataByScore(mem, gameAssembly.base, 0x200000u, 8192, 15.0, false, 0, 0x200000u, metaBytes))
    {
        CloseHandle(hProc);
        return false;
    }

    MetadataHeaderFields header;
    if (!ReadMetadataHeaderFieldsFromBytes(metaBytes, header))
    {
        CloseHandle(hProc);
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
    (void)DumpSdk6GetMetadataRegistrationTypes(mem, hint.metadataRegistration, header.version, typesPtr, typesCount, fieldOffsetsPtr);

    const std::string outDir = JoinPathA(GetExeDirA(), "DumpSDK");
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(outDir), ec);

    const std::string dumpCsPath = JoinPathA(outDir, "dump.cs");
    const std::string genericJsonPath = JoinPathA(outDir, "generic.json");

    if (!DumpSdk6WriteGenericJsonFile(genericJsonPath, header, metaBytes, genericParams, typeFullName))
    {
        CloseHandle(hProc);
        return false;
    }

    if (!DumpSdk6WriteDumpCsFile(dumpCsPath, mem, hint, header, metaBytes, images, typeToImage, typeFullName, typeMap, genericParams, typesPtr, typesCount, fieldOffsetsPtr))
    {
        CloseHandle(hProc);
        return false;
    }

    outPaths.outDir = outDir;
    outPaths.dumpCsPath = dumpCsPath;
    outPaths.genericJsonPath = genericJsonPath;

    CloseHandle(hProc);
    return true;
}

} // namespace er2
