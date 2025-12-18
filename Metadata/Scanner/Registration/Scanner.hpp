#pragma once

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <vector>

#include "../../../Core/UnityExternalMemory.hpp"
#include "Types.hpp"
#include "Helpers.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

inline bool FindCodeRegistration(const IMemoryAccessor& mem,
                                 std::uintptr_t moduleBase,
                                 std::uint32_t moduleSize,
                                 const std::filesystem::path& modulePath,
                                 std::uintptr_t metaBase,
                                 std::size_t chunkSize,
                                 double maxSeconds,
                                 std::uintptr_t& outAddr)
{
    outAddr = 0;
    if (moduleBase == 0 || moduleSize == 0 || metaBase == 0) return false;

    std::vector<DiskSection> secs;
    if (!GetDiskPeSections(modulePath, secs)) return false;

    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> execRanges;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> dataRanges;
    std::vector<DiskSection> dataSecs;
    BuildRanges(moduleBase, secs, execRanges, dataRanges, dataSecs);

    std::uint32_t imagesSize = 0;
    if (!ReadValue(mem, metaBase + 0xACu, imagesSize)) return false;
    if (imagesSize == 0 || (imagesSize % 0x28u) != 0u) return false;
    std::uint32_t imageCount = imagesSize / 0x28u;
    if (imageCount == 0 || imageCount > 300000u) return false;

    std::uint32_t version = 0;
    if (!ReadValue(mem, metaBase + 0x04u, version)) return false;

    CodeRegOffsets off = GetCodeRegistrationOffsets(version);
    if (off.needBytes <= 0) return false;

    std::uintptr_t moduleEnd = moduleBase + (std::uintptr_t)moduleSize;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(maxSeconds);

    std::vector<std::uint8_t> buf;
    for (const auto& sec : dataSecs)
    {
        if (std::chrono::steady_clock::now() > deadline) break;
        if (sec.vsize == 0) continue;

        std::uintptr_t start = moduleBase + (std::uintptr_t)sec.rva;
        std::uint32_t size = sec.vsize;

        std::uint32_t offset = 0;
        while (offset < size && std::chrono::steady_clock::now() <= deadline)
        {
            std::uint32_t remain = size - offset;
            std::size_t toRead = remain > (std::uint32_t)chunkSize ? chunkSize : (std::size_t)remain;
            if (!ReadChunk(mem, start + (std::uintptr_t)offset, toRead, buf))
            {
                offset += (std::uint32_t)toRead;
                continue;
            }
            if (buf.size() < (std::size_t)off.needBytes)
            {
                offset += (std::uint32_t)toRead;
                continue;
            }

            std::size_t scanEnd = buf.size() - (std::size_t)off.needBytes;
            for (std::size_t i = 0; i <= scanEnd; i += 8)
            {
                std::uintptr_t baseAddr = start + (std::uintptr_t)offset + (std::uintptr_t)i;

                std::uint64_t cgCnt = U64At(buf, i + (std::size_t)off.codeGenModulesCount);
                if (cgCnt != (std::uint64_t)imageCount) continue;
                std::uint64_t cgPtr = U64At(buf, i + (std::size_t)off.codeGenModules);
                if (cgPtr == 0) continue;
                if (!CheckCodeGenModulesArray(mem, moduleBase, moduleEnd, (std::uintptr_t)cgPtr, 3)) continue;

                std::uint64_t invCnt = U64At(buf, i + (std::size_t)off.invokerPointersCount);
                std::uint64_t invPtr = U64At(buf, i + (std::size_t)off.invokerPointers);
                if (invCnt == 0 || invCnt > 10000000ull) continue;
                if (!CheckPointerArrayPointsIntoExec(mem, (std::uintptr_t)invPtr, moduleBase, moduleEnd, execRanges, 3)) continue;

                outAddr = baseAddr;
                return true;
            }

            offset += (std::uint32_t)toRead;
        }
    }

    return false;
}

inline bool FindMetadataRegistration(const IMemoryAccessor& mem,
                                     std::uintptr_t moduleBase,
                                     std::uint32_t moduleSize,
                                     const std::filesystem::path& modulePath,
                                     std::uintptr_t metaBase,
                                     std::size_t chunkSize,
                                     double maxSeconds,
                                     std::uintptr_t& outAddr)
{
    outAddr = 0;
    if (moduleBase == 0 || moduleSize == 0 || metaBase == 0) return false;

    std::vector<DiskSection> secs;
    if (!GetDiskPeSections(modulePath, secs)) return false;

    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> execRanges;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> dataRanges;
    std::vector<DiskSection> dataSecs;
    BuildRanges(moduleBase, secs, execRanges, dataRanges, dataSecs);

    std::uint32_t version = 0;
    if (!ReadValue(mem, metaBase + 0x04u, version)) return false;

    MetaRegOffsets off = GetMetadataRegistrationOffsets(version);
    if (off.structSize <= 0) return false;

    std::uint32_t typeDefSize = 0;
    if (!ReadValue(mem, metaBase + 0xA4u, typeDefSize)) return false;

    std::vector<std::int64_t> typeDefCounts = InferTypeDefCounts(typeDefSize);
    if (typeDefCounts.empty()) return false;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(maxSeconds);

    std::vector<std::uint8_t> buf;
    for (const auto& sec : dataSecs)
    {
        if (std::chrono::steady_clock::now() > deadline) break;
        if (sec.vsize == 0) continue;

        std::uintptr_t start = moduleBase + (std::uintptr_t)sec.rva;
        std::uint32_t size = sec.vsize;

        std::uint32_t offset = 0;
        while (offset < size && std::chrono::steady_clock::now() <= deadline)
        {
            std::uint32_t remain = size - offset;
            std::size_t toRead = remain > (std::uint32_t)chunkSize ? chunkSize : (std::size_t)remain;
            if (!ReadChunk(mem, start + (std::uintptr_t)offset, toRead, buf))
            {
                offset += (std::uint32_t)toRead;
                continue;
            }
            if (buf.size() < (std::size_t)off.structSize)
            {
                offset += (std::uint32_t)toRead;
                continue;
            }

            std::size_t scanEnd = buf.size() - (std::size_t)off.structSize;
            for (std::size_t i = 0; i <= scanEnd; i += 8)
            {
                std::uintptr_t baseAddr = start + (std::uintptr_t)offset + (std::uintptr_t)i;

                std::int64_t fieldOffsetsCnt = I64At(buf, i + (std::size_t)off.fieldOffsetsCount);
                std::int64_t typeSizesCnt = I64At(buf, i + (std::size_t)off.typeDefinitionsSizesCount);
                if (fieldOffsetsCnt != typeSizesCnt) continue;
                if (!HasCountCandidate(typeDefCounts, fieldOffsetsCnt)) continue;

                std::int64_t typesCnt = I64At(buf, i + (std::size_t)off.typesCount);
                if (typesCnt <= 0 || typesCnt > 300000) continue;

                std::uint64_t typesPtr = U64At(buf, i + (std::size_t)off.types);
                std::uint64_t fieldOffsetsPtr = U64At(buf, i + (std::size_t)off.fieldOffsets);
                std::uint64_t typeSizesPtr = U64At(buf, i + (std::size_t)off.typeDefinitionsSizes);
                if (typesPtr == 0 || fieldOffsetsPtr == 0 || typeSizesPtr == 0) continue;
                if (!InAny((std::uintptr_t)typesPtr, dataRanges)) continue;
                if (!InAny((std::uintptr_t)fieldOffsetsPtr, dataRanges)) continue;
                if (!InAny((std::uintptr_t)typeSizesPtr, dataRanges)) continue;

                bool ok = true;
                for (int j = 0; j < 3; ++j)
                {
                    std::uintptr_t p = 0;
                    if (!ReadPtr(mem, (std::uintptr_t)typesPtr + (std::uintptr_t)j * 8u, p))
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
                if (!ok) continue;

                int nonzero = 0;
                for (int j = 0; j < 3; ++j)
                {
                    std::uintptr_t p = 0;
                    if (!ReadPtr(mem, (std::uintptr_t)fieldOffsetsPtr + (std::uintptr_t)j * 8u, p))
                    {
                        ok = false;
                        break;
                    }
                    if (p == 0) continue;
                    ++nonzero;
                    if (!InAny(p, dataRanges))
                    {
                        ok = false;
                        break;
                    }
                }
                if (!ok || nonzero == 0) continue;

                outAddr = baseAddr;
                return true;
            }

            offset += (std::uint32_t)toRead;
        }
    }

    return false;
}

inline bool FindIl2CppRegistrations(const IMemoryAccessor& mem,
                                   std::uintptr_t moduleBase,
                                   std::uint32_t moduleSize,
                                   const std::filesystem::path& modulePath,
                                   std::uintptr_t metaBase,
                                   std::size_t chunkSize,
                                   double maxSeconds,
                                   Il2CppRegs& out)
{
    out.codeRegistration = 0;
    out.metadataRegistration = 0;

    std::uintptr_t cr = 0;
    std::uintptr_t mr = 0;
    (void)FindCodeRegistration(mem, moduleBase, moduleSize, modulePath, metaBase, chunkSize, maxSeconds, cr);
    (void)FindMetadataRegistration(mem, moduleBase, moduleSize, modulePath, metaBase, chunkSize, maxSeconds, mr);
    out.codeRegistration = cr;
    out.metadataRegistration = mr;
    return (cr != 0) || (mr != 0);
}

}

}
