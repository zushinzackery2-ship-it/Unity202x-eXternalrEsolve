#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>

#include <er2/unity2/dumpsdk/dump_log.hpp>

#include <filesystem>

namespace er2
{

bool SidecarWriter::WriteAll(
    const std::string& outputDir,
    const CollectedData& data,
    std::uintptr_t moduleBase)
{
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(outputDir), error);
    DumpSdkLog(DumpSdkLogLevel::Info, "[Sidecar] WriteAll begin: " + outputDir);

    bool ok = true;
    ok &= WriteDumpCs(outputDir + "\\dump.cs", data, moduleBase);
    ok &= WriteIl2CppHeader(outputDir + "\\il2cpp.h", data);
    ok &= WriteScriptJson(outputDir + "\\script.json", data, moduleBase);
    ok &= WriteStringLiteralJson(outputDir + "\\stringliteral.json", data, moduleBase);

    DumpSdkLog(ok ? DumpSdkLogLevel::Info : DumpSdkLogLevel::Error,
        ok ? "[Sidecar] WriteAll ok" : "[Sidecar] WriteAll failed");
    return ok;
}

} // namespace er2
