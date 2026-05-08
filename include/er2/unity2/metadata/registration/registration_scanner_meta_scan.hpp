#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../../../mem/memory_read.hpp"
#include "../../dumpsdk/sdk_registration.hpp"
#include "registration_helpers.hpp"
#include "../pe.hpp"

namespace er2
{

namespace detail_il2cpp_reg
{

// 从ModuleSection转换为DiskSection
inline void ConvertModuleSectionsToDiskSections(const std::vector<ModuleSection>& modSecs, std::vector<DiskSection>& out)
{
    out.clear();
    out.reserve(modSecs.size());
    for (const auto& ms : modSecs)
    {
        DiskSection ds;
        std::memset(ds.name, 0, sizeof(ds.name));
        std::strncpy(ds.name, ms.name.c_str(), 8);
        ds.rva = ms.rva;
        ds.vsize = ms.size;
        ds.rawPtr = 0;
        ds.rawSize = 0;
        out.push_back(ds);
    }
}

inline bool FindMetadataRegistrationFromMemory(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
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

    std::vector<ModuleSection> modSecs;
    std::uint32_t sizeOfImage = 0;
    if (!ReadModuleSections(mem, moduleBase, sizeOfImage, modSecs))
    {
        return false;
    }

    std::vector<DiskSection> secs;
    ConvertModuleSectionsToDiskSections(modSecs, secs);

    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> execRanges;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> dataRanges;
    std::vector<DiskSection> dataSecs;
    BuildRanges(moduleBase, secs, execRanges, dataRanges, dataSecs);

    std::uint32_t version = 0;
    if (!ReadValue(mem, metaBase + 0x04u, version))
    {
        return false;
    }

    const MetaRegOffsets off = GetMetadataRegistrationOffsets(version);
    if (off.structSize <= 0)
    {
        return false;
    }

    std::uint32_t typeDefSize = 0;
    if (!ReadValue(mem, metaBase + 0xA4u, typeDefSize))
    {
        return false;
    }

    const std::vector<std::int64_t> typeDefCounts = InferTypeDefCounts(typeDefSize);
    if (typeDefCounts.empty())
    {
        return false;
    }

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
            if (buf.size() < static_cast<std::size_t>(off.structSize))
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }

            const std::size_t scanEnd = buf.size() - static_cast<std::size_t>(off.structSize);
            for (std::size_t i = 0; i <= scanEnd; i += 8)
            {
                const std::uintptr_t baseAddr = start + static_cast<std::uintptr_t>(offset) + static_cast<std::uintptr_t>(i);

                const std::int64_t fieldOffsetsCnt = I64At(buf, i + static_cast<std::size_t>(off.fieldOffsetsCount));
                const std::int64_t typeSizesCnt = I64At(buf, i + static_cast<std::size_t>(off.typeDefinitionsSizesCount));
                if (fieldOffsetsCnt != typeSizesCnt)
                {
                    continue;
                }
                if (!HasCountCandidate(typeDefCounts, fieldOffsetsCnt))
                {
                    continue;
                }

                const std::int64_t typesCnt = I64At(buf, i + static_cast<std::size_t>(off.typesCount));
                if (typesCnt <= 0 || typesCnt > 300000)
                {
                    continue;
                }

                const std::uint64_t typesPtr = U64At(buf, i + static_cast<std::size_t>(off.types));
                const std::uint64_t fieldOffsetsPtr = U64At(buf, i + static_cast<std::size_t>(off.fieldOffsets));
                const std::uint64_t typeSizesPtr = U64At(buf, i + static_cast<std::size_t>(off.typeDefinitionsSizes));
                if (typesPtr == 0 || fieldOffsetsPtr == 0 || typeSizesPtr == 0)
                {
                    continue;
                }
                if (!InAny(static_cast<std::uintptr_t>(typesPtr), dataRanges))
                {
                    continue;
                }
                if (!InAny(static_cast<std::uintptr_t>(fieldOffsetsPtr), dataRanges))
                {
                    continue;
                }
                if (!InAny(static_cast<std::uintptr_t>(typeSizesPtr), dataRanges))
                {
                    continue;
                }

                bool ok = true;
                for (int j = 0; j < 3; ++j)
                {
                    std::uintptr_t p = 0;
                    if (!ReadPtr(mem, static_cast<std::uintptr_t>(typesPtr) + static_cast<std::uintptr_t>(j) * 8u, p))
                    {
                        ok = false;
                        break;
                    }
                    if (p == 0 || !InAny(p, dataRanges))
                    {
                        ok = false;
                        break;
                    }
                }
                if (!ok)
                {
                    continue;
                }

                int nonzero = 0;
                for (int j = 0; j < 3; ++j)
                {
                    std::uintptr_t p = 0;
                    if (!ReadPtr(mem, static_cast<std::uintptr_t>(fieldOffsetsPtr) + static_cast<std::uintptr_t>(j) * 8u, p))
                    {
                        ok = false;
                        break;
                    }
                    if (p == 0)
                    {
                        continue;
                    }
                    ++nonzero;
                    if (!InAny(p, dataRanges))
                    {
                        ok = false;
                        break;
                    }
                }
                if (!ok || nonzero == 0)
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

inline bool FindMetadataRegistrationSidecarStyle(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
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

    std::uint32_t version = 0;
    if (!ReadValue(mem, metaBase + 0x04u, version))
    {
        return false;
    }

    const MetaRegOffsets off = GetMetadataRegistrationOffsets(version);
    if (off.fieldOffsetsCount < 0 ||
        off.typeDefinitionsSizesCount < 0 ||
        off.typeDefinitionsSizes < 0)
    {
        return false;
    }

    if (off.typeDefinitionsSizesCount < off.fieldOffsetsCount ||
        off.typeDefinitionsSizes < off.fieldOffsetsCount)
    {
        return false;
    }

    std::uint32_t typeDefSize = 0;
    if (!ReadValue(mem, metaBase + 0xA4u, typeDefSize))
    {
        return false;
    }

    const std::vector<std::int64_t> typeDefCounts = InferTypeDefCounts(typeDefSize);
    if (typeDefCounts.empty())
    {
        return false;
    }

    std::vector<ModuleSection> modSecs;
    std::uint32_t sizeOfImage = 0;
    if (!ReadModuleSections(mem, moduleBase, sizeOfImage, modSecs))
    {
        return false;
    }

    std::vector<DiskSection> secs;
    ConvertModuleSectionsToDiskSections(modSecs, secs);

    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> execRanges;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> dataRanges;
    std::vector<DiskSection> dataSecs;
    BuildRanges(moduleBase, secs, execRanges, dataRanges, dataSecs);

    const std::size_t typeDefinitionsSizesCountDelta =
        static_cast<std::size_t>(off.typeDefinitionsSizesCount - off.fieldOffsetsCount);
    const std::size_t typeDefinitionsSizesDelta =
        static_cast<std::size_t>(off.typeDefinitionsSizes - off.fieldOffsetsCount);
    const std::size_t requiredTail = typeDefinitionsSizesDelta + sizeof(std::uint64_t);
    const std::size_t effectiveChunkSize = chunkSize == 0 ? 0x20000u : chunkSize;
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
            const std::size_t toRead =
                remain > static_cast<std::uint32_t>(effectiveChunkSize)
                    ? effectiveChunkSize
                    : static_cast<std::size_t>(remain);
            if (!ReadChunk(mem, start + static_cast<std::uintptr_t>(offset), toRead, buf))
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }

            if (buf.size() <= requiredTail)
            {
                offset += static_cast<std::uint32_t>(toRead);
                continue;
            }

            const std::size_t scanEnd = buf.size() - requiredTail;
            for (std::size_t i = 0; i <= scanEnd; i += sizeof(std::uint64_t))
            {
                const std::int64_t fieldOffsetsCnt = I64At(buf, i);
                if (!HasCountCandidate(typeDefCounts, fieldOffsetsCnt))
                {
                    continue;
                }

                const std::int64_t typeSizesCnt = I64At(buf, i + typeDefinitionsSizesCountDelta);
                if (typeSizesCnt != fieldOffsetsCnt)
                {
                    continue;
                }

                const std::uint64_t typeSizesPtr = U64At(buf, i + typeDefinitionsSizesDelta);
                if (typeSizesPtr == 0 || !InAny(static_cast<std::uintptr_t>(typeSizesPtr), dataRanges))
                {
                    continue;
                }

                const std::uintptr_t fieldOffsetsCountAddr =
                    start + static_cast<std::uintptr_t>(offset) + static_cast<std::uintptr_t>(i);
                if (fieldOffsetsCountAddr < static_cast<std::uintptr_t>(off.fieldOffsetsCount))
                {
                    continue;
                }

                const std::uintptr_t candidate =
                    fieldOffsetsCountAddr - static_cast<std::uintptr_t>(off.fieldOffsetsCount);

                std::uintptr_t typesPtr = 0;
                std::uint32_t typesCount = 0;
                std::uintptr_t fieldOffsetsPtr = 0;
                if (!DumpSdk6GetMetadataRegistrationTypes(
                        mem,
                        candidate,
                        version,
                        typesPtr,
                        typesCount,
                        fieldOffsetsPtr) ||
                    typesPtr == 0 ||
                    fieldOffsetsPtr == 0 ||
                    typesCount == 0 ||
                    typesCount > 300000u)
                {
                    continue;
                }

                if (!InAny(typesPtr, dataRanges) || !InAny(fieldOffsetsPtr, dataRanges))
                {
                    continue;
                }

                outAddr = candidate;
                return true;
            }

            offset += static_cast<std::uint32_t>(toRead);
        }
    }

    return false;
}

}

}
