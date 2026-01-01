#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "registration_helpers.hpp"

namespace er2
{

namespace detail_il2cpp_reg
{

inline bool FindCodeRegistration(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const std::filesystem::path& modulePath,
    std::uintptr_t metaBase,
    std::size_t chunkSize,
    double maxSeconds,
    std::uintptr_t& outAddr)
{
    outAddr = 0;
    if (moduleBase == 0 || moduleSize == 0 || metaBase == 0)
    {
        return false;
    }

    std::vector<DiskSection> secs;
    if (!GetDiskPeSections(modulePath, secs))
    {
        return false;
    }

    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> execRanges;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> dataRanges;
    std::vector<DiskSection> dataSecs;
    BuildRanges(moduleBase, secs, execRanges, dataRanges, dataSecs);

    std::uint32_t imagesSize = 0;
    if (!ReadValue(mem, metaBase + 0xACu, imagesSize))
    {
        return false;
    }
    if (imagesSize == 0 || (imagesSize % 0x28u) != 0u)
    {
        return false;
    }
    const std::uint32_t imageCount = imagesSize / 0x28u;
    if (imageCount == 0 || imageCount > 300000u)
    {
        return false;
    }

    std::uint32_t version = 0;
    if (!ReadValue(mem, metaBase + 0x04u, version))
    {
        return false;
    }

    const CodeRegOffsets off = GetCodeRegistrationOffsets(version);
    if (off.needBytes <= 0)
    {
        return false;
    }

    const std::uintptr_t moduleEnd = moduleBase + static_cast<std::uintptr_t>(moduleSize);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(maxSeconds);

    std::vector<std::uint8_t> buf;
    for (const auto& sec : dataSecs)
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            break;
        }
        if (sec.vsize == 0)
        {
            continue;
        }

        const std::uintptr_t start = moduleBase + static_cast<std::uintptr_t>(sec.rva);
        const std::uint32_t size = sec.vsize;

        std::uint32_t offset = 0;
        while (offset < size && std::chrono::steady_clock::now() <= deadline)
        {
            const std::uint32_t remain = size - offset;
            const std::size_t toRead = remain > static_cast<std::uint32_t>(chunkSize) ? chunkSize : static_cast<std::size_t>(remain);
            if (!ReadChunk(mem, start + static_cast<std::uintptr_t>(offset), toRead, buf))
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }
            if (buf.size() < static_cast<std::size_t>(off.needBytes))
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }

            const std::size_t scanEnd = buf.size() - static_cast<std::size_t>(off.needBytes);
            for (std::size_t i = 0; i <= scanEnd; i += 8)
            {
                const std::uintptr_t baseAddr = start + static_cast<std::uintptr_t>(offset) + static_cast<std::uintptr_t>(i);

                const std::uint64_t cgCnt = U64At(buf, i + static_cast<std::size_t>(off.codeGenModulesCount));
                if (cgCnt != static_cast<std::uint64_t>(imageCount))
                {
                    continue;
                }

                const std::uint64_t cgPtr = U64At(buf, i + static_cast<std::size_t>(off.codeGenModules));
                if (cgPtr == 0)
                {
                    continue;
                }
                if (!CheckCodeGenModulesArray(mem, moduleBase, moduleEnd, static_cast<std::uintptr_t>(cgPtr), 3))
                {
                    continue;
                }

                const std::uint64_t invCnt = U64At(buf, i + static_cast<std::size_t>(off.invokerPointersCount));
                const std::uint64_t invPtr = U64At(buf, i + static_cast<std::size_t>(off.invokerPointers));
                if (invCnt == 0 || invCnt > 10000000ull)
                {
                    continue;
                }
                if (!CheckPointerArrayPointsIntoExec(mem, static_cast<std::uintptr_t>(invPtr), moduleBase, moduleEnd, execRanges, 3))
                {
                    continue;
                }

                outAddr = baseAddr;
                return true;
            }

            offset += static_cast<std::uint32_t>(toRead);
        }
    }

    return false;
}

}

}
