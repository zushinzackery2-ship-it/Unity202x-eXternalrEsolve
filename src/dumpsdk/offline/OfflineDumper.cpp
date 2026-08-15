#include <er2/unity2/dumpsdk/offline/OfflineDumper.h>

#include <er2/os/win/local_memory_accessor.hpp>
#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/dump_progress.hpp>
#include <er2/unity2/dumpsdk/offline/OfflineCollector.h>
#include "OfflineArtifactWriter.h"
#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>
#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefExporter.h>
#include <er2/unity2/metadata.hpp>

#include <Psapi.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <utility>
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

    DumpSdkProgressScope locateProgress("Locate metadata", 1);
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
    locateProgress.Complete();

    DumpSdkProgressScope readProgress("Read metadata", 1);
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
    readProgress.Complete();

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

} // namespace

bool DumpIl2CppOfflineCollect(
    const IMemoryAccessor& mem,
    const PeImage& pe,
    const std::string& outDir,
    CollectedData& data,
    GlobalStringXrefAnalysis* xrefAnalysis,
    std::string& error)
{
    data = {};
    const std::uintptr_t moduleBase = static_cast<std::uintptr_t>(pe.ImageBase());
    const std::size_t snapshotSize = pe.Size();
    if (!pe.IsBound()
        || !pe.IsMemoryLoaded()
        || moduleBase == 0
        || snapshotSize == 0
        || snapshotSize > (std::numeric_limits<std::uint32_t>::max)())
    {
        error = "module snapshot invalid";
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
    if (!OfflineArtifactWriter::WriteMetadata(metadataPath, metadataBytes))
    {
        error = "failed to write global-metadata.dat";
        DumpSdkLog(DumpSdkLogLevel::Error, "[Il2CppOffline] " + error);
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=collect");
    RegistrationInitResult registration{};
    if (!Collect(pe,
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

    if (!OfflineArtifactWriter::WriteHint(
            outputDir,
            moduleBase,
            registration.codeRegistrationVa,
            registration.metadataRegistrationVa))
    {
        error = "failed to write il2cpp-offline.hint.json";
        DumpSdkLog(DumpSdkLogLevel::Error, "[Il2CppOffline] " + error);
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=global-string-xrefs");
    GlobalStringXrefReportResults xrefResults;
    std::string xrefError;
    GlobalStringXrefAnalysis analysis = GlobalStringXrefExporter::Analyze(pe, {});
    if (!GlobalStringXrefExporter::WriteReports(
            pe,
            analysis,
            outputDir,
            xrefResults,
            xrefError))
    {
        error = "global string xref export failed: " + xrefError;
        DumpSdkLog(DumpSdkLogLevel::Error, "[Il2CppOffline] " + error);
        return false;
    }
    DumpSdkLog(DumpSdkLogLevel::Info, std::format(
        "[Il2CppOffline] global string xrefs globalStrings={} globalReferences={} "
        "runtimeRdataStrings={} runtimeRdataReferences={}",
        xrefResults.global.stringCount,
        xrefResults.global.referenceCount,
        xrefResults.runtimeRdata.stringCount,
        xrefResults.runtimeRdata.referenceCount));

    if (xrefAnalysis != nullptr)
    {
        *xrefAnalysis = std::move(analysis);
    }
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=end");
    return true;
}

bool DumpIl2CppOfflineCollect(
    const IMemoryAccessor& mem,
    const std::uintptr_t moduleBase,
    const std::uint32_t moduleSize,
    const std::string& outDir,
    CollectedData& data,
    std::string& error)
{
    if (moduleBase == 0 || moduleSize == 0)
    {
        error = "moduleBase/moduleSize invalid";
        return false;
    }

    PeImage snapshot;
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] stage=snapshot");
    if (!snapshot.LoadFromModuleRange(moduleBase, moduleSize, error))
    {
        DumpSdkLog(
            DumpSdkLogLevel::Error,
            "[Il2CppOffline] module snapshot failed: " + error);
        return false;
    }
    return DumpIl2CppOfflineCollect(
        mem,
        snapshot,
        outDir,
        data,
        nullptr,
        error);
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
