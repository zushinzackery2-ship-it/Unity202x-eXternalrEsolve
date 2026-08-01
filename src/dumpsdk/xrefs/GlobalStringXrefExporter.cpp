#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefExporter.h>

#include "GlobalStringCatalog.h"
#include "GlobalStringXrefDocumentWriter.h"
#include "X64ReferenceScanner.h"
#include "XrefPeAccess.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace er2
{

namespace
{

constexpr const char* GlobalReportName = "global-string-xrefs.json";
constexpr const char* RuntimeRdataReportName = "runtime-rdata-string-xrefs.json";
constexpr const char* RuntimeRdataSection = ".rdata";

struct XrefAnalysis
{
    GlobalStringXrefOptions options;
    std::vector<DetectedGlobalString> strings;
    std::vector<std::size_t> referencedIndices;
};

bool HasReference(
    const DetectedGlobalString& detected,
    const GlobalStringReferenceCandidate& candidate)
{
    return std::any_of(
        detected.references.begin(),
        detected.references.end(),
        [&candidate](const GlobalStringReference& reference)
        {
            return reference.rva == candidate.instructionRva
                && reference.kind == candidate.kind;
        });
}

GlobalStringXrefOptions NormalizeOptions(const GlobalStringXrefOptions& requestedOptions)
{
    GlobalStringXrefOptions options = requestedOptions;
    options.minimumLength = (std::max<std::size_t>)(2, options.minimumLength);
    options.maximumByteLength = (std::max)(options.minimumLength, options.maximumByteLength);
    return options;
}

XrefAnalysis AnalyzeSnapshot(
    const PeImage& image,
    const GlobalStringXrefOptions& requestedOptions)
{
    XrefAnalysis analysis;
    analysis.options = NormalizeOptions(requestedOptions);

    const std::vector<DetectedGlobalString> catalog = GlobalStringCatalog::Extract(
        image,
        analysis.options);
    analysis.strings = catalog;
    std::unordered_map<std::uintptr_t, std::size_t> exactStrings;
    exactStrings.reserve(analysis.strings.size());
    for (std::size_t index = 0; index < analysis.strings.size(); ++index)
    {
        exactStrings.emplace(analysis.strings[index].address, index);
    }

    std::unordered_set<std::size_t> referencedIndices;
    for (const GlobalStringReferenceCandidate& candidate : X64ReferenceScanner::Scan(image))
    {
        std::size_t stringIndex = 0;
        const auto exact = exactStrings.find(candidate.targetAddress);
        if (exact != exactStrings.end())
        {
            stringIndex = exact->second;
        }
        else
        {
            std::size_t containingIndex = 0;
            if (!GlobalStringCatalog::FindContaining(catalog, candidate.targetAddress, containingIndex))
            {
                continue;
            }

            DetectedGlobalString detected;
            if (!GlobalStringCatalog::TryDecodeAtAddress(
                    image,
                    candidate.targetAddress,
                    analysis.options,
                    detected))
            {
                continue;
            }
            stringIndex = analysis.strings.size();
            analysis.strings.push_back(std::move(detected));
            exactStrings.emplace(candidate.targetAddress, stringIndex);
        }

        DetectedGlobalString& detected = analysis.strings[stringIndex];
        if (!HasReference(detected, candidate))
        {
            detected.references.push_back({
                candidate.instructionRva,
                candidate.targetAddress,
                candidate.section,
                candidate.kind,
                candidate.mnemonic });
        }
        referencedIndices.insert(stringIndex);
    }

    analysis.referencedIndices.assign(referencedIndices.begin(), referencedIndices.end());
    for (const std::size_t index : analysis.referencedIndices)
    {
        std::sort(
            analysis.strings[index].references.begin(),
            analysis.strings[index].references.end(),
            [](const GlobalStringReference& left, const GlobalStringReference& right)
            {
                return left.rva < right.rva;
            });
    }
    std::sort(
        analysis.referencedIndices.begin(),
        analysis.referencedIndices.end(),
        [&analysis](std::size_t left, std::size_t right)
        {
            return analysis.strings[left].rva < analysis.strings[right].rva;
        });
    return analysis;
}

std::vector<std::size_t> FilterBySection(
    const XrefAnalysis& analysis,
    const char* sectionName)
{
    std::vector<std::size_t> result;
    result.reserve(analysis.referencedIndices.size());
    for (const std::size_t index : analysis.referencedIndices)
    {
        if (XrefEqualsIgnoreCase(analysis.strings[index].section, sectionName))
        {
            result.push_back(index);
        }
    }
    return result;
}

} // namespace

bool GlobalStringXrefExporter::ExportReports(
    const PeImage& image,
    const std::filesystem::path& outputDirectory,
    const GlobalStringXrefOptions& options,
    GlobalStringXrefReportResults& results,
    std::string& error)
{
    results = {};
    XrefAnalysis analysis = AnalyzeSnapshot(image, options);
    if (!GlobalStringXrefDocumentWriter::Write(
            image,
            outputDirectory / GlobalReportName,
            analysis.options,
            analysis.strings,
            analysis.referencedIndices,
            nullptr,
            results.global,
            error))
    {
        return false;
    }

    const std::vector<std::size_t> runtimeRdataIndices = FilterBySection(
        analysis,
        RuntimeRdataSection);
    return GlobalStringXrefDocumentWriter::Write(
        image,
        outputDirectory / RuntimeRdataReportName,
        analysis.options,
        analysis.strings,
        runtimeRdataIndices,
        RuntimeRdataSection,
        results.runtimeRdata,
        error);
}

bool GlobalStringXrefExporter::ExportReportsFromModule(
    HMODULE runtimeModule,
    const std::filesystem::path& outputDirectory,
    const GlobalStringXrefOptions& options,
    GlobalStringXrefReportResults& results,
    std::string& error)
{
    PeImage image;
    if (!image.LoadFromModuleSnapshot(runtimeModule, error))
    {
        return false;
    }
    return ExportReports(image, outputDirectory, options, results, error);
}

} // namespace er2
