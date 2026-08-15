#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace er2::OfflineArtifactWriter
{

bool WriteHint(
    const std::filesystem::path& outputDirectory,
    std::uintptr_t moduleBase,
    std::uintptr_t codeRegistrationVa,
    std::uintptr_t metadataRegistrationVa);

bool WriteMetadata(
    const std::filesystem::path& metadataPath,
    const std::vector<std::uint8_t>& metadataBytes);

bool RemoveLegacyXrefReports(
    const std::filesystem::path& outputDirectory,
    std::string& error);

} // namespace er2::OfflineArtifactWriter
