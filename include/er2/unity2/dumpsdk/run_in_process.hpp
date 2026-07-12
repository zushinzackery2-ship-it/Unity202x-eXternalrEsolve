#pragma once

#include <cstdint>
#include <string>

#include "../../mem/memory_accessor.hpp"

#include "collected_data.hpp"
#include "dump_log.hpp"
#include "offline/OfflineDumper.h"
#include "sdk_runner.hpp"
#include "writers/dummy_dll_generator.hpp"
#include "writers/sidecar_writer.hpp"

namespace er2
{

struct DumpSdkInProcessResult
{
    std::string outDir;
    std::string error;
    bool ok = false;
};

/// In-process dump pipeline: Collect → Sidecar → DummyDll.
/// DumpSdkDump is optional enhancement for generic.json (failure does not fail main path).
inline bool DumpSdkRunInProcess(
    const IMemoryAccessor& mem,
    std::uintptr_t gameAssemblyBase,
    std::uint32_t gameAssemblySize,
    const std::string& outDir,
    DumpSdkInProcessResult& result)
{
    result = DumpSdkInProcessResult{};
    result.outDir = outDir;

    CollectedData data;
    std::string error;
    DumpSdkLog(DumpSdkLogLevel::Info, "[DumpSdkRunInProcess] stage=collect");
    if (!DumpIl2CppOfflineCollect(mem, gameAssemblyBase, gameAssemblySize, outDir, data, error))
    {
        result.error = std::move(error);
        result.ok = false;
        DumpSdkLog(DumpSdkLogLevel::Error, "[DumpSdkRunInProcess] collect failed: " + result.error);
        return false;
    }

    size_t typeCount = 0;
    for (const auto& asm_ : data.assemblies)
    {
        typeCount += asm_.types.size();
    }
    if (typeCount == 0)
    {
        result.error = "no types collected";
        result.ok = false;
        DumpSdkLog(DumpSdkLogLevel::Error, "[DumpSdkRunInProcess] " + result.error);
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        "[DumpSdkRunInProcess] stage=sidecar types=" + std::to_string(typeCount));
    if (!SidecarWriter::WriteAll(outDir, data, gameAssemblyBase))
    {
        result.error = "Sidecar WriteAll failed";
        result.ok = false;
        DumpSdkLog(DumpSdkLogLevel::Error, "[DumpSdkRunInProcess] " + result.error);
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[DumpSdkRunInProcess] stage=dummydll");
    if (!DummyDllGenerator::Generate(outDir, data))
    {
        result.error = "DummyDll Generate failed";
        result.ok = false;
        DumpSdkLog(DumpSdkLogLevel::Error, "[DumpSdkRunInProcess] " + result.error);
        return false;
    }

    // Sidecar already owns dump.cs; only enhance with generic.json (+ metadata/hint refresh).
    DumpSdkLog(DumpSdkLogLevel::Info, "[DumpSdkRunInProcess] stage=optional-generic.json");
    DumpSdk6Paths paths;
    std::string dumpError;
    if (!DumpSdkDump(mem, gameAssemblyBase, gameAssemblySize, outDir, paths, &dumpError, false))
    {
        DumpSdkLog(DumpSdkLogLevel::Warn,
            "[DumpSdkRunInProcess] generic.json optional failed (main path kept): " + dumpError);
    }
    else
    {
        DumpSdkLog(DumpSdkLogLevel::Info, "[DumpSdkRunInProcess] generic.json optional ok");
    }

    result.outDir = outDir;
    result.ok = true;
    DumpSdkLog(DumpSdkLogLevel::Info, "[DumpSdkRunInProcess] stage=end ok");
    return true;
}

} // namespace er2
