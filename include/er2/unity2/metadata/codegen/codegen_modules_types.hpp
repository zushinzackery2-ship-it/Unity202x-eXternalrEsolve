#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "../../../mem/memory_read.hpp"

#include "../hint/hint_struct.hpp" // CodeGenModuleHint

#include "../pe.hpp"         // ModuleSection

#include "../registration/registration_helpers.hpp" // disk section helpers + range checks + chunk reader

#include "../registration/registration_types.hpp"   // CodeRegOffsets

namespace er2
{

namespace detail_codegen_modules
{

inline bool EqualsIgnoreCaseAscii(const std::string& a, const char* b)
{
    if (!b)
    {
        return false;
    }
    const std::size_t blen = std::strlen(b);
    if (a.size() != blen)
    {
        return false;
    }
    for (std::size_t i = 0; i < blen; ++i)
    {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
        if (ca != cb)
        {
            return false;
        }
    }
    return true;
}

inline bool IsLikelyCodeGenModuleName(const std::string& s)
{
    if (s.empty() || s.size() > 260)
    {
        return false;
    }
    if (s == "__Generated")
    {
        return true;
    }
    return detail_il2cpp_reg::ContainsDllCaseInsensitive(s);
}

inline void BuildRangesFromModuleSections(
    std::uintptr_t moduleBase,
    const std::vector<ModuleSection>& secs,
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& dataRanges,
    std::vector<ModuleSection>& dataSecs)
{
    execRanges.clear();
    dataRanges.clear();
    dataSecs.clear();

    // Heuristic ranges used for pointer validation and scanning.
    // Mirror the Python strategy:
    // - execRanges: .text + il2cpp
    // - dataRanges: everything except .text
    // - dataSecs (scanned): common data-like sections + il2cpp
    for (const auto& s : secs)
    {
        if (s.size == 0)
        {
            continue;
        }

        const std::uintptr_t start = moduleBase + static_cast<std::uintptr_t>(s.rva);
        const std::uintptr_t end = start + static_cast<std::uintptr_t>(s.size);

        const bool isText = EqualsIgnoreCaseAscii(s.name, ".text");
        const bool isIl2Cpp = EqualsIgnoreCaseAscii(s.name, "il2cpp") || EqualsIgnoreCaseAscii(s.name, ".il2cpp");

        if (isText || isIl2Cpp)
        {
            execRanges.push_back(std::make_pair(start, end));
        }

        if (!isText)
        {
            dataRanges.push_back(std::make_pair(start, end));
        }

        if (EqualsIgnoreCaseAscii(s.name, ".data") || EqualsIgnoreCaseAscii(s.name, ".rdata") || EqualsIgnoreCaseAscii(s.name, "_rdata") || EqualsIgnoreCaseAscii(s.name, ".pdata") || EqualsIgnoreCaseAscii(s.name, ".tls") || EqualsIgnoreCaseAscii(s.name, ".reloc") || isIl2Cpp)
        {
            dataSecs.push_back(s);
        }
    }
}

} // namespace detail_codegen_modules

} // namespace er2
