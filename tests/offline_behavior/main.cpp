#include "SmokeAssertions.h"
#include "SmokeMetadataBuilder.h"
#include "SmokePeBuilder.h"

#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/offline/OfflineCollector.h>
#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>
#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>

#include <filesystem>
#include <format>
#include <iostream>

namespace
{

uint64_t CountTypes(const er2::CollectedData& data)
{
    uint64_t count = 0;
    for (const er2::CollectedAssembly& assembly : data.assemblies)
    {
        count += assembly.types.size();
    }
    return count;
}

uint64_t CountMethods(const er2::CollectedData& data)
{
    uint64_t count = 0;
    for (const er2::CollectedAssembly& assembly : data.assemblies)
    {
        for (const er2::CollectedType& type : assembly.types)
        {
            count += type.methods.size();
        }
    }
    return count;
}

bool ClearRunFileOffset(er2::CollectedData& data)
{
    for (er2::CollectedAssembly& assembly : data.assemblies)
    {
        for (er2::CollectedType& type : assembly.types)
        {
            if (type.name != "Derived")
            {
                continue;
            }
            for (er2::CollectedMethod& method : type.methods)
            {
                if (method.name == "Run")
                {
                    method.fileOffset = 0;
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace

int main()
{
    using namespace OfflineBehavior;

    er2::SetDumpSdkLogCallback([](er2::DumpSdkLogLevel, const std::string& message)
    {
        std::cout << "  [log] " << message << "\n";
    });

    Checks checks;
    std::cout << "-- Building synthetic v29 metadata + PE module\n";
    const MetadataBlob metadata = BuildMetadata();
    checks.Ok(metadata.bytes.size() > 0x12000, "metadata blob built");

    SmokeModule module{};
    std::string error;
    if (!BuildSmokeModule(metadata, module, error))
    {
        std::cout << "  [FATAL] " << error << "\n";
        return 1;
    }

    std::cout << "-- er2::Collect\n";
    er2::CollectedData data{};
    er2::RegistrationInitResult registration{};
    if (!er2::Collect(
            module.base,
            module.size,
            metadata.bytes.data(),
            metadata.bytes.size(),
            module.metadataAddress,
            data,
            error,
            &registration))
    {
        std::cout << "  [FATAL] Collect failed: " << error << "\n";
        ReleaseSmokeModule(module);
        return 1;
    }

    checks.Equal(registration.codeRegistrationVa, module.codeRegistrationVa, "CodeRegistration located");
    checks.Equal(registration.metadataRegistrationVa, module.metadataRegistrationVa, "MetadataRegistration located");
    checks.Equal(CountTypes(data), static_cast<uint64_t>(kTypeCount), "every type collected");
    checks.Equal(CountMethods(data), static_cast<uint64_t>(kMethodCount), "every method collected");
    CheckCollectedGraph(checks, data, module);

    const std::string outputDir = "out";
    std::filesystem::create_directories(outputDir);
    if (!er2::SidecarWriter::WriteAll(outputDir, data, module.base))
    {
        std::cout << "  [FATAL] SidecarWriter::WriteAll failed\n";
        ReleaseSmokeModule(module);
        return 1;
    }

    const std::string dump = ReadTextFile(outputDir + "/dump.cs");
    checks.Ok(!dump.empty(), "dump.cs written");
    CheckDumpCs(checks, dump, module);
    CheckSidecars(checks, outputDir);

    er2::CollectedData apiData = data;
    apiData.fromOffline = false;
    checks.Ok(ClearRunFileOffset(apiData), "API fallback fixture selected Derived.Run");
    const std::string apiOutputDir = "out_api";
    std::filesystem::create_directories(apiOutputDir);
    const bool apiWritten = er2::SidecarWriter::WriteAll(apiOutputDir, apiData, module.base);
    checks.Ok(apiWritten, "API-style sidecars written");
    if (apiWritten)
    {
        const std::string apiDump = ReadTextFile(apiOutputDir + "/dump.cs");
        checks.Contains(apiDump,
            std::format(
                "// RVA: 0x{:X} Offset: 0x{:X} VA:",
                module.MethodRva(kMethodDerivedRun),
                module.MethodRva(kMethodDerivedRun)),
            "API path falls back from missing fileOffset to RVA");
    }

    ReleaseSmokeModule(module);
    return checks.Report();
}
