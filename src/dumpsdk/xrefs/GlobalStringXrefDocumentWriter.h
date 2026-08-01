#pragma once

#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefTypes.h>

#include <filesystem>
#include <string>
#include <vector>

namespace er2
{

class GlobalStringXrefDocumentWriter
{
public:
    static bool Write(
        const PeImage& image,
        const std::filesystem::path& outputPath,
        const GlobalStringXrefOptions& options,
        const std::vector<DetectedGlobalString>& strings,
        const std::vector<std::size_t>& outputIndices,
        const char* sectionFilter,
        GlobalStringXrefResult& result,
        std::string& error);
};

} // namespace er2
