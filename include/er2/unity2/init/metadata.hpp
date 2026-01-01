#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "context.hpp"


#include "../metadata/export.hpp"
#include "../metadata/header_parser.hpp"
#include "../metadata/pe.hpp"
#include "../metadata/scanner_pointer.hpp"
#include "../metadata/hint_export.hpp"
#include "../metadata/registration_scanner.hpp"
#include "../metadata/codegen_modules.hpp"
#include "../metadata/method_resolver.hpp"

namespace er2
{

inline bool TryGetGameAssemblyModuleInfo(ModuleInfo& out)
{
    out = ModuleInfo{};
    if (!IsInited())
    {
        return false;
    }

    return er2::GetRemoteModuleInfo(g_ctx.pid, L"GameAssembly.dll", out);
}

inline std::optional<ModuleInfo> TryGetGameAssemblyModuleInfo()
{
    ModuleInfo out;
    if (!TryGetGameAssemblyModuleInfo(out))
    {
        return std::nullopt;
    }
    return out;
}

inline bool ExportGameAssemblyMetadataByScore(std::vector<std::uint8_t>& out)
{
    out.clear();

    if (!IsInited())
    {
        return false;
    }

    ModuleInfo ga;
    if (!TryGetGameAssemblyModuleInfo(ga) || !ga.base)
    {
        return false;
    }

    return er2::ExportMetadataByScore(
        Mem(),
        ga.base,
        0x200000u,
        8192,
        15.0,
        false,
        0,
        0x200000u,
        out);
}

inline std::optional<std::vector<std::uint8_t>> ExportGameAssemblyMetadataByScore()
{
    std::vector<std::uint8_t> out;
    if (!ExportGameAssemblyMetadataByScore(out))
    {
        return std::nullopt;
    }
    return out;
}

inline bool ExportGameAssemblyMetadataHintJsonTScoreToSidecar(const std::filesystem::path& outDatPath)
{
    if (!IsInited())
    {
        return false;
    }

    ModuleInfo ga;
    if (!TryGetGameAssemblyModuleInfo(ga) || !ga.base)
    {
        return false;
    }

    return er2::ExportMetadataHintJsonTScoreToSidecar(
        Mem(),
        outDatPath,
        ga.base,
        g_ctx.pid,
        L"",
        L"GameAssembly.dll");
}

} // namespace er2
