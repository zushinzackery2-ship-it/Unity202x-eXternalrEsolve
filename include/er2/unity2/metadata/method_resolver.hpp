#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "hint_struct.hpp"
#include "metadata_images.hpp"
#include "registration_helpers.hpp" // Disk PE section parsing (RVA->file offset)
#include "../util/assembly_name.hpp"

namespace er2
{

struct ResolvedMethodAddress
{
    std::string imageName;
    std::string codeGenModuleName;

    std::uintptr_t va = 0;
    std::uint64_t rva = 0;
    std::uint64_t fileOffset = 0;
};

namespace detail_method_resolve
{

inline std::uint32_t TokenRowId(std::uint32_t token)
{
    return token & 0x00FFFFFFu;
}

inline const CodeGenModuleHint* FindCodeGenModuleForImage(const MetadataHint& hint, const std::string& imageName)
{
    if (hint.codeGenModuleList.empty())
    {
        return nullptr;
    }

    const std::string key = er2::NormalizeAssemblyKey(imageName);
    if (key.empty())
    {
        return nullptr;
    }

    const CodeGenModuleHint* best = nullptr;
    for (const auto& m : hint.codeGenModuleList)
    {
        const std::string mk = er2::NormalizeAssemblyKey(m.name);
        if (mk.empty())
        {
            continue;
        }
        if (mk == key)
        {
            best = &m;
            break;
        }
    }

    // Fallback: if no exact match, keep nullptr (caller may choose another strategy).
    return best;
}

inline bool TryRvaToFileOffset(
    std::uint32_t rva,
    const std::vector<detail_il2cpp_reg::DiskSection>& secs,
    std::uint64_t& out)
{
    out = 0;
    if (rva == 0)
    {
        return false;
    }

    for (const auto& s : secs)
    {
        if (s.vsize == 0)
        {
            continue;
        }
        const std::uint32_t start = s.rva;
        const std::uint32_t end = start + s.vsize;
        if (rva < start || rva >= end)
        {
            continue;
        }

        if (s.rawPtr == 0)
        {
            return false;
        }

        const std::uint64_t off = static_cast<std::uint64_t>(s.rawPtr) + static_cast<std::uint64_t>(rva - start);
        out = off;
        return true;
    }

    return false;
}

} // namespace detail_method_resolve

inline bool ResolveMethodAddress(
    const IMemoryAccessor& mem,
    const MetadataHint& hint,
    std::uint32_t typeDefIndex,
    std::uint32_t methodToken,
    ResolvedMethodAddress& out)
{
    out = ResolvedMethodAddress{};

    if (!hint.metaBase || !hint.moduleBase || hint.moduleSize == 0)
    {
        return false;
    }

    std::vector<std::string> typeToImage;
    if (!BuildTypeDefIndexToImageNameFromMemory(mem, hint.metaBase, typeToImage))
    {
        return false;
    }
    if (typeDefIndex >= typeToImage.size())
    {
        return false;
    }

    const std::string imageName = typeToImage[typeDefIndex];
    if (imageName.empty())
    {
        return false;
    }

    const CodeGenModuleHint* mod = detail_method_resolve::FindCodeGenModuleForImage(hint, imageName);
    if (!mod || mod->methodPointers == 0 || mod->methodPointerCount == 0)
    {
        return false;
    }

    const std::uint32_t rowId = detail_method_resolve::TokenRowId(methodToken);
    if (rowId == 0)
    {
        return false;
    }

    const std::uint64_t idx = static_cast<std::uint64_t>(rowId - 1u);
    if (idx >= static_cast<std::uint64_t>(mod->methodPointerCount))
    {
        return false;
    }

    std::uintptr_t fn = 0;
    if (!ReadPtr(mem, mod->methodPointers + static_cast<std::uintptr_t>(idx) * 8u, fn) || fn == 0)
    {
        return false;
    }

    out.imageName = imageName;
    out.codeGenModuleName = mod->name;
    out.va = fn;

    if (fn >= hint.moduleBase)
    {
        out.rva = static_cast<std::uint64_t>(fn - hint.moduleBase);
    }

    // Optional file offset (requires modulePath to be available).
    if (!hint.modulePath.empty() && out.rva != 0)
    {
        std::vector<detail_il2cpp_reg::DiskSection> secs;
        if (detail_il2cpp_reg::GetDiskPeSections(std::filesystem::path(hint.modulePath), secs))
        {
            (void)detail_method_resolve::TryRvaToFileOffset(static_cast<std::uint32_t>(out.rva), secs, out.fileOffset);
        }
    }

    return true;
}

} // namespace er2
