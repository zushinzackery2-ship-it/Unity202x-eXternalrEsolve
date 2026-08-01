#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>

#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/dump_progress.hpp>

#include <filesystem>

namespace er2
{

bool SidecarWriter::WriteAll(
    const std::string& outputDir,
    const CollectedData& data,
    std::uintptr_t moduleBase)
{
    DumpSdkProgressScope progress("Write Sidecar files", 4);
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(outputDir), error);
    DumpSdkLog(DumpSdkLogLevel::Info, "[Sidecar] WriteAll begin: " + outputDir);

    bool ok = true;
    ok &= WriteDumpCs(outputDir + "\\dump.cs", data, moduleBase);
    progress.Update(1, "dump.cs");
    ok &= WriteIl2CppHeader(outputDir + "\\il2cpp.h", data);
    progress.Update(2, "il2cpp.h");
    ok &= WriteScriptJson(outputDir + "\\script.json", data, moduleBase);
    progress.Update(3, "script.json");
    ok &= WriteStringLiteralJson(outputDir + "\\stringliteral.json", data, moduleBase);
    progress.Update(4, "stringliteral.json");

    DumpSdkLog(ok ? DumpSdkLogLevel::Info : DumpSdkLogLevel::Error,
        ok ? "[Sidecar] WriteAll ok" : "[Sidecar] WriteAll failed");
    if (ok)
    {
        progress.Complete();
    }
    else
    {
        progress.Fail("one or more Sidecar files failed");
    }
    return ok;
}

} // namespace er2
