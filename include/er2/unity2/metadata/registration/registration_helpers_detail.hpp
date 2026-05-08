#pragma once

#include "../../../os/win/win_include.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "../../../mem/memory_read.hpp"
#include "string_utils.hpp"

namespace er2
{

namespace detail_il2cpp_reg
{

inline bool CheckCodeGenModulesArray(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uintptr_t moduleEnd,
    std::uintptr_t codeGenModules,
    int sample)
{
    if (codeGenModules == 0)
    {
        return false;
    }
    if (codeGenModules < moduleBase || codeGenModules >= moduleEnd)
    {
        return false;
    }

    for (int i = 0; i < sample; ++i)
    {
        std::uintptr_t pmod = 0;
        if (!ReadPtr(mem, codeGenModules + static_cast<std::uintptr_t>(i) * 8u, pmod))
        {
            return false;
        }
        if (pmod == 0 || pmod < moduleBase || pmod >= moduleEnd)
        {
            return false;
        }

        std::uintptr_t moduleNamePtr = 0;
        if (!ReadPtr(mem, pmod + 0u, moduleNamePtr))
        {
            return false;
        }

        std::string s;
        if (!ReadCString(mem, moduleNamePtr, s, 260))
        {
            return false;
        }
        if (s.empty())
        {
            return false;
        }
        if (!ContainsDllCaseInsensitive(s))
        {
            return false;
        }
    }

    return true;
}

inline bool CheckPointerArrayPointsIntoExec(
    const IMemoryAccessor& mem,
    std::uintptr_t ptr,
    std::uintptr_t moduleBase,
    std::uintptr_t moduleEnd,
    const std::vector<std::pair<std::uintptr_t, std::uintptr_t>>& execRanges,
    int sample)
{
    if (ptr == 0 || ptr < moduleBase || ptr >= moduleEnd)
    {
        return false;
    }

    for (int i = 0; i < sample; ++i)
    {
        std::uintptr_t p = 0;
        if (!ReadPtr(mem, ptr + static_cast<std::uintptr_t>(i) * 8u, p))
        {
            return false;
        }
        if (p == 0)
        {
            return false;
        }
        if (!InAny(p, execRanges))
        {
            return false;
        }
    }

    return true;
}

inline std::vector<std::int64_t> InferTypeDefCounts(std::uint32_t typeDefSize)
{
    static const std::uint32_t candidates[] = {
        0x58, 0x54, 0x50, 0x4C, 0x48, 0x44, 0x40, 0x3C, 0x38, 0x34, 0x30, 0x2C, 0x28, 0x24, 0x20,
    };

    std::vector<std::int64_t> out;
    if (typeDefSize == 0)
    {
        return out;
    }

    for (std::uint32_t sz : candidates)
    {
        if ((typeDefSize % sz) != 0)
        {
            continue;
        }
        const std::uint32_t cnt = typeDefSize / sz;
        if (cnt == 0 || cnt > 300000u)
        {
            continue;
        }
        out.push_back(static_cast<std::int64_t>(cnt));
    }
    return out;
}

inline bool HasCountCandidate(const std::vector<std::int64_t>& candidates, std::int64_t value)
{
    for (std::int64_t c : candidates)
    {
        if (c == value)
        {
            return true;
        }
    }
    return false;
}

}

}
