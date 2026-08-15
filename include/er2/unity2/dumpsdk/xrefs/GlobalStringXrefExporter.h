#pragma once

#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefTypes.h>

#include <Windows.h>
#include <filesystem>
#include <string>

namespace er2
{

class GlobalStringXrefExporter
{
public:
    static GlobalStringXrefAnalysis Analyze(
        const PeImage& image,
        const GlobalStringXrefOptions& options);

    static bool WriteReports(
        const PeImage& image,
        const GlobalStringXrefAnalysis& analysis,
        const std::filesystem::path& outputDirectory,
        GlobalStringXrefReportResults& results,
        std::string& error);

    static bool ExportReports(
        const PeImage& image,
        const std::filesystem::path& outputDirectory,
        const GlobalStringXrefOptions& options,
        GlobalStringXrefReportResults& results,
        std::string& error);

    static bool ExportReportsFromModule(
        HMODULE runtimeModule,
        const std::filesystem::path& outputDirectory,
        const GlobalStringXrefOptions& options,
        GlobalStringXrefReportResults& results,
        std::string& error);
};

} // namespace er2
