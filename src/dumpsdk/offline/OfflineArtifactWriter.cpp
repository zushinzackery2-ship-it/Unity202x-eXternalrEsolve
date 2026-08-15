#include "OfflineArtifactWriter.h"

#include <er2/unity2/dumpsdk/dump_progress.hpp>

#include <algorithm>
#include <format>
#include <fstream>

namespace er2::OfflineArtifactWriter
{
namespace
{

bool RemoveReport(
    const std::filesystem::path& path,
    std::string& error)
{
    std::error_code filesystemError;
    std::filesystem::remove(path, filesystemError);
    if (!filesystemError)
    {
        return true;
    }
    error = "failed to remove legacy XRef report "
        + path.string() + ": " + filesystemError.message();
    return false;
}

} // namespace

bool WriteHint(
    const std::filesystem::path& outputDirectory,
    const std::uintptr_t moduleBase,
    const std::uintptr_t codeRegistrationVa,
    const std::uintptr_t metadataRegistrationVa)
{
    DumpSdkProgressScope progress(
        "Write offline hint",
        1,
        "il2cpp-offline.hint.json");
    const std::filesystem::path hintPath =
        outputDirectory / "il2cpp-offline.hint.json";
    std::ofstream output(hintPath);
    if (!output)
    {
        return false;
    }

    const std::uint64_t codeRva = codeRegistrationVa >= moduleBase
        ? codeRegistrationVa - moduleBase
        : 0;
    const std::uint64_t metadataRva = metadataRegistrationVa >= moduleBase
        ? metadataRegistrationVa - moduleBase
        : 0;
    output << "{\n"
        << "  \"module\": {\n"
        << "    \"base_addr\": \"0x" << std::hex << moduleBase << "\",\n"
        << "    \"code_registration_rva\": \"0x" << codeRva << "\",\n"
        << "    \"metadata_registration_rva\": \"0x" << metadataRva << "\"\n"
        << "  }\n"
        << "}\n";
    if (!output)
    {
        return false;
    }
    progress.Complete();
    return true;
}

bool WriteMetadata(
    const std::filesystem::path& metadataPath,
    const std::vector<std::uint8_t>& metadataBytes)
{
    DumpSdkProgressScope progress(
        "Write metadata file",
        metadataBytes.size(),
        "global-metadata.dat");
    std::ofstream output(
        metadataPath,
        std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }

    constexpr std::size_t writeChunkSize = 1024u * 1024u;
    std::size_t offset = 0;
    while (offset < metadataBytes.size())
    {
        const std::size_t writeSize = (std::min)(
            writeChunkSize,
            metadataBytes.size() - offset);
        output.write(
            reinterpret_cast<const char*>(metadataBytes.data() + offset),
            static_cast<std::streamsize>(writeSize));
        if (!output)
        {
            return false;
        }
        offset += writeSize;
        progress.Update(offset);
    }
    progress.Complete();
    return true;
}

bool RemoveLegacyXrefReports(
    const std::filesystem::path& outputDirectory,
    std::string& error)
{
    return RemoveReport(outputDirectory / "global-string-xrefs.json", error)
        && RemoveReport(
            outputDirectory / "runtime-rdata-string-xrefs.json",
            error);
}

} // namespace er2::OfflineArtifactWriter
