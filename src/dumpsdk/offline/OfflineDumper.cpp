#include <er2/unity2/dumpsdk/offline/OfflineDumper.h>

#include <er2/os/win/local_memory_accessor.hpp>
#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/offline/OfflineCollector.h>
#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>
#include <er2/unity2/metadata.hpp>

#include <Psapi.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

namespace er2
{

namespace
{

struct MetadataScanResult
{
    uintptr_t metaBase = 0;
    size_t totalSize = 0;
};

bool TryExtractMetadataFromModule(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::vector<uint8_t>& outBytes,
    MetadataScanResult& scan)
{
    outBytes.clear();
    scan = {};

    if (moduleBase == 0)
    {
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        "[Il2CppOffline] metadata scan: er2 FindMetadataPointerByScore");

    FoundMetadata found = FindMetadataByScore(
        mem,
        moduleBase,
        0x200000u,
        8192,
        180.0,
        false,
        0);

    if (!found.metaBase)
    {
        DumpSdkLog(DumpSdkLogLevel::Warn,
            "[Il2CppOffline] er2 metadata pointer score search failed");
        return false;
    }

    std::uint32_t totalSize = 0;
    if (!CalcTotalSizeFromHeader(mem, found.metaBase, totalSize) || totalSize == 0)
    {
        DumpSdkLog(DumpSdkLogLevel::Warn,
            "[Il2CppOffline] er2 CalcTotalSizeFromHeader failed");
        return false;
    }

    if (!ReadMetadataRegion(mem, found.metaBase, totalSize, 0x200000u, outBytes))
    {
        DumpSdkLog(DumpSdkLogLevel::Warn,
            "[Il2CppOffline] er2 ReadMetadataRegion failed");
        return false;
    }

    scan.metaBase = found.metaBase;
    scan.totalSize = outBytes.size();

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] er2 metadata ok base=0x{:X} ptr=0x{:X} score={} size={}",
            found.metaBase,
            found.ptrAddr,
            found.score,
            scan.totalSize));
    return true;
}

bool WriteHintJson(const std::filesystem::path& outputDir,
    uintptr_t moduleBase,
    uintptr_t codeRegistrationVa,
    uintptr_t metadataRegistrationVa)
{
    const auto hintPath = outputDir / "il2cpp-offline.hint.json";
    std::ofstream out(hintPath);
    if (!out)
    {
        return false;
    }
    const uint64_t codeRva = codeRegistrationVa >= moduleBase ? codeRegistrationVa - moduleBase : 0;
    const uint64_t metaRva = metadataRegistrationVa >= moduleBase ? metadataRegistrationVa - moduleBase : 0;
    out << "{\n";
    out << "  \"module\": {\n";
    out << "    \"base_addr\": \"0x" << std::hex << moduleBase << "\",\n";
    out << "    \"code_registration_rva\": \"0x" << codeRva << "\",\n";
    out << "    \"metadata_registration_rva\": \"0x" << metaRva << "\"\n";
    out << "  }\n";
    out << "}\n";
    return true;
}

} // namespace

bool DumpIl2CppOfflineCollect(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const std::string& outDir,
    CollectedData& data,
    std::string& error)
{
    data = {};
    if (moduleBase == 0 || moduleSize == 0)
    {
        error = "moduleBase/moduleSize invalid";
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=begin");

    const std::filesystem::path outputDir(outDir);
    std::vector<uint8_t> metadataBytes;
    MetadataScanResult scan{};
    const auto metadataPath = outputDir / "global-metadata.dat";

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=extract-metadata");
    if (!TryExtractMetadataFromModule(mem, moduleBase, metadataBytes, scan))
    {
        error = "Metadata scan failed: could not locate global-metadata.dat in memory";
        DumpSdkLog(DumpSdkLogLevel::Error, "[Il2CppOffline] " + error);
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Extracted global-metadata.dat from memory: {} bytes @ 0x{:X}",
            metadataBytes.size(),
            scan.metaBase));
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    std::ofstream outFile(metadataPath, std::ios::binary);
    if (outFile)
    {
        outFile.write(reinterpret_cast<const char*>(metadataBytes.data()),
            static_cast<std::streamsize>(metadataBytes.size()));
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=collect");
    RegistrationInitResult registration{};
    if (!Collect(moduleBase,
        moduleSize,
        metadataBytes.data(),
        metadataBytes.size(),
        scan.metaBase,
        data,
        error,
        &registration))
    {
        DumpSdkLog(DumpSdkLogLevel::Error,
            "[Il2CppOffline] collect failed: " + error);
        return false;
    }

    WriteHintJson(outputDir,
        moduleBase,
        registration.codeRegistrationVa,
        registration.metadataRegistrationVa);

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=end");
    return true;
}

bool DumpIl2CppOfflineCollectFromModule(
    HMODULE runtimeModule,
    const std::string& outDir,
    CollectedData& data,
    std::string& error)
{
    data = {};
    if (runtimeModule == nullptr)
    {
        error = "runtimeModule is null";
        return false;
    }

    MODULEINFO modInfo{};
    if (!GetModuleInformation(GetCurrentProcess(), runtimeModule, &modInfo, sizeof(modInfo)))
    {
        error = "GetModuleInformation failed";
        return false;
    }

    LocalMemoryAccessor mem;
    return DumpIl2CppOfflineCollect(
        mem,
        reinterpret_cast<std::uintptr_t>(modInfo.lpBaseOfDll),
        static_cast<std::uint32_t>(modInfo.SizeOfImage),
        outDir,
        data,
        error);
}

} // namespace er2
