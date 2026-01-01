#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "hint_struct.hpp" // CodeGenModuleHint
#include "pe.hpp"         // ModuleSection
#include "registration_helpers.hpp" // disk section helpers + range checks + chunk reader
#include "registration_types.hpp"   // CodeRegOffsets

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

inline bool TryReadCodeGenModulesFromCodeRegistration(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uintptr_t moduleEnd,
    const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
    const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& dataRanges,
    std::uint32_t metaVersion,
    std::uintptr_t codeRegistration,
    std::uint32_t expectedImageCount,
    std::uintptr_t& outArray,
    std::uint32_t& outCount,
    std::vector<CodeGenModuleHint>& outList)
{
    outArray = 0;
    outCount = 0;
    outList.clear();

    if (!codeRegistration || codeRegistration < moduleBase || codeRegistration >= moduleEnd)
    {
        return false;
    }

    const detail_il2cpp_reg::CodeRegOffsets off = detail_il2cpp_reg::GetCodeRegistrationOffsets(metaVersion);
    if (off.needBytes <= 0)
    {
        return false;
    }

    std::uint64_t cgCnt64 = 0;
    std::uint64_t cgPtr64 = 0;
    if (!ReadValue(mem, codeRegistration + static_cast<std::uintptr_t>(off.codeGenModulesCount), cgCnt64))
    {
        return false;
    }
    if (!ReadValue(mem, codeRegistration + static_cast<std::uintptr_t>(off.codeGenModules), cgPtr64))
    {
        return false;
    }

    if (cgCnt64 == 0 || cgCnt64 > 1000000ull)
    {
        return false;
    }
    if (expectedImageCount != 0 && cgCnt64 != static_cast<std::uint64_t>(expectedImageCount))
    {
        // Not fatal: some builds can disagree, but still attempt to read.
    }

    const std::uintptr_t cgPtr = static_cast<std::uintptr_t>(cgPtr64);
    if (cgPtr == 0)
    {
        return false;
    }
    if (!detail_il2cpp_reg::InAny(cgPtr, dataRanges))
    {
        return false;
    }

    outArray = cgPtr;
    outCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(cgCnt64, 1000000ull));

    const std::uint32_t maxModules = std::min<std::uint32_t>(outCount, 10000u);
    outList.reserve(maxModules);

    for (std::uint32_t i = 0; i < maxModules; ++i)
    {
        std::uintptr_t mod = 0;
        if (!ReadPtr(mem, cgPtr + static_cast<std::uintptr_t>(i) * 8u, mod))
        {
            break;
        }
        if (mod == 0 || !detail_il2cpp_reg::InAny(mod, dataRanges))
        {
            continue;
        }

        std::uintptr_t namePtr = 0;
        if (!ReadPtr(mem, mod + 0u, namePtr) || namePtr == 0 || !detail_il2cpp_reg::InAny(namePtr, dataRanges))
        {
            continue;
        }

        std::string name;
        if (!ReadCString(mem, namePtr, name, 260))
        {
            continue;
        }
        if (!IsLikelyCodeGenModuleName(name))
        {
            continue;
        }

        std::uint32_t mcount = 0;
        if (!ReadValue(mem, mod + 0x08u, mcount))
        {
            continue;
        }
        if (mcount == 0 || mcount > 2000000u)
        {
            continue;
        }

        std::uintptr_t mp = 0;
        if (!ReadPtr(mem, mod + 0x10u, mp) || mp == 0 || !detail_il2cpp_reg::InAny(mp, dataRanges))
        {
            continue;
        }

        // Validate a couple of pointers are executable.
        bool ok = true;
        for (int j = 0; j < 3 && j < static_cast<int>(mcount); ++j)
        {
            std::uintptr_t fn = 0;
            if (!ReadPtr(mem, mp + static_cast<std::uintptr_t>(j) * 8u, fn))
            {
                ok = false;
                break;
            }
            if (fn != 0 && !detail_il2cpp_reg::InAny(fn, execRanges))
            {
                ok = false;
                break;
            }
        }
        if (!ok)
        {
            continue;
        }

        CodeGenModuleHint h;
        h.name = name;
        h.address = mod;
        h.methodPointerCount = mcount;
        h.methodPointers = mp;
        outList.push_back(std::move(h));
    }

    return !outList.empty();
}

inline bool TryScanCodeGenModulesHeuristic(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    const std::vector<ModuleSection>& dataSecs,
    const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
    const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& dataRanges,
    std::size_t chunkSize,
    double maxSeconds,
    std::vector<CodeGenModuleHint>& outList)
{
    outList.clear();
    if (!moduleBase || dataSecs.empty())
    {
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(maxSeconds);
    const std::size_t readSize = std::max<std::size_t>(chunkSize, 0x2000u);

    std::vector<std::uint8_t> buf;

    for (const auto& sec : dataSecs)
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            break;
        }
        if (sec.size == 0)
        {
            continue;
        }

        const std::uintptr_t start = moduleBase + static_cast<std::uintptr_t>(sec.rva);
        const std::uint32_t size = sec.size;

        std::uint32_t offset = 0;
        while (offset < size && std::chrono::steady_clock::now() <= deadline)
        {
            const std::uint32_t remain = size - offset;
            const std::size_t toRead = remain > static_cast<std::uint32_t>(readSize) ? readSize : static_cast<std::size_t>(remain);
            if (!detail_il2cpp_reg::ReadChunk(mem, start + static_cast<std::uintptr_t>(offset), toRead, buf))
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }
            if (buf.size() < 0x20u)
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }

            const std::size_t limit = buf.size() - 0x20u;
            for (std::size_t i = 0; i <= limit; i += 8)
            {
                const std::uint64_t namePtr64 = detail_il2cpp_reg::U64At(buf, i + 0u);
                if (namePtr64 == 0 || !detail_il2cpp_reg::InAny(static_cast<std::uintptr_t>(namePtr64), dataRanges))
                {
                    continue;
                }

                const std::uint32_t mcount = static_cast<std::uint32_t>(detail_il2cpp_reg::U64At(buf, i + 0x08u) & 0xFFFFFFFFu);
                if (mcount == 0 || mcount > 2000000u)
                {
                    continue;
                }

                const std::uint64_t mpPtr64 = detail_il2cpp_reg::U64At(buf, i + 0x10u);
                if (mpPtr64 == 0 || !detail_il2cpp_reg::InAny(static_cast<std::uintptr_t>(mpPtr64), dataRanges))
                {
                    continue;
                }

                std::string name;
                if (!ReadCString(mem, static_cast<std::uintptr_t>(namePtr64), name, 260))
                {
                    continue;
                }
                if (!IsLikelyCodeGenModuleName(name))
                {
                    continue;
                }

                bool ok = true;
                for (int j = 0; j < 3 && j < static_cast<int>(mcount); ++j)
                {
                    std::uintptr_t fn = 0;
                    if (!ReadPtr(mem, static_cast<std::uintptr_t>(mpPtr64) + static_cast<std::uintptr_t>(j) * 8u, fn))
                    {
                        ok = false;
                        break;
                    }
                    if (fn != 0 && !detail_il2cpp_reg::InAny(fn, execRanges))
                    {
                        ok = false;
                        break;
                    }
                }
                if (!ok)
                {
                    continue;
                }

                CodeGenModuleHint h;
                h.name = name;
                h.address = start + static_cast<std::uintptr_t>(offset) + static_cast<std::uintptr_t>(i);
                h.methodPointerCount = mcount;
                h.methodPointers = static_cast<std::uintptr_t>(mpPtr64);
                outList.push_back(std::move(h));
            }

            offset += static_cast<std::uint32_t>(toRead);
        }
    }

    // De-dupe by name (keep first hit).
    if (!outList.empty())
    {
        std::vector<CodeGenModuleHint> uniq;
        uniq.reserve(outList.size());
        for (const auto& m : outList)
        {
            bool seen = false;
            for (const auto& u : uniq)
            {
                if (u.name == m.name)
                {
                    seen = true;
                    break;
                }
            }
            if (!seen)
            {
                uniq.push_back(m);
            }
        }
        outList.swap(uniq);
    }

    return !outList.empty();
}

} // namespace detail_codegen_modules

inline bool CollectCodeGenModules(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const std::vector<ModuleSection>& moduleSections,
    std::uint32_t metaVersion,
    std::uint32_t imagesCount,
    std::uintptr_t codeRegistration,
    std::uintptr_t& outArray,
    std::uint32_t& outCount,
    std::vector<CodeGenModuleHint>& outList,
    std::size_t scanChunkSize = 0x200000u,
    double scanMaxSeconds = 2.5)
{
    outArray = 0;
    outCount = 0;
    outList.clear();

    if (!moduleBase || moduleSize == 0)
    {
        return false;
    }

    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> execRanges;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> dataRanges;
    std::vector<ModuleSection> dataSecs;
    detail_codegen_modules::BuildRangesFromModuleSections(moduleBase, moduleSections, execRanges, dataRanges, dataSecs);

    if (execRanges.empty() || dataRanges.empty())
    {
        return false;
    }

    const std::uintptr_t moduleEnd = moduleBase + static_cast<std::uintptr_t>(moduleSize);

    bool ok = false;
    if (codeRegistration && metaVersion != 0)
    {
        ok = detail_codegen_modules::TryReadCodeGenModulesFromCodeRegistration(
            mem,
            moduleBase,
            moduleEnd,
            execRanges,
            dataRanges,
            metaVersion,
            codeRegistration,
            imagesCount,
            outArray,
            outCount,
            outList);
    }

    if (!ok && !dataSecs.empty())
    {
        if (detail_codegen_modules::TryScanCodeGenModulesHeuristic(mem, moduleBase, dataSecs, execRanges, dataRanges, scanChunkSize, scanMaxSeconds, outList))
        {
            outArray = 0;
            outCount = static_cast<std::uint32_t>(outList.size());
            ok = true;
        }
    }

    return ok;
}

} // namespace er2
