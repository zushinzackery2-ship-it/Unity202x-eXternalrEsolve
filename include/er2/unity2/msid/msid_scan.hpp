#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../../os/win/win_module.hpp"
#include "../core/offsets.hpp"
#include "../metadata/pe.hpp"
#include "../gom/gom_offsets.hpp"
#include "../gom/gom_walker.hpp"
#include "ms_id_to_pointer.hpp"
#include "../../mem/memory_read.hpp"

namespace er2
{

inline bool IsReadableByte(const IMemoryAccessor& mem, std::uintptr_t address)
{
    if (!address)
    {
        return false;
    }

    std::uint8_t b = 0;
    return mem.Read(address, &b, 1);
}

// Validate Unity Object via Managed<->Native cross-reference
inline bool ValidateUnityObject(
    const IMemoryAccessor& mem,
    std::uintptr_t obj,
    const GomOffsets& gomOff)
{
    if (!IsCanonicalUserPtr(obj))
    {
        return false;
    }

    std::uintptr_t managed = 0;
    if (!GetManagedFromNative(mem, obj, gomOff, managed))
    {
        return true;
    }

    if (!IsCanonicalUserPtr(managed))
    {
        return false;
    }

    constexpr std::size_t kManagedNativeOffset = 0x10;
    std::uintptr_t native = 0;
    if (!ReadPtr(mem, managed + kManagedNativeOffset, native))
    {
        return false;
    }

    return native == obj;
}

inline std::uint32_t CountUnityObjectsInMsIdEntriesPool(
    const IMemoryAccessor& mem,
    std::uintptr_t entriesBase,
    std::uint32_t capacity,
    std::uint32_t expectedCount,
    const GomOffsets& gomOff)
{
    if (!IsCanonicalUserPtr(entriesBase))
    {
        return 0;
    }
    if (expectedCount == 0 || expectedCount > 5000000)
    {
        return 0;
    }
    if (capacity == 0 || capacity > 20000000)
    {
        return 0;
    }

    constexpr std::uint32_t kBatchSize = 10000;
    std::vector<std::uint8_t> batch;
    batch.resize(static_cast<std::size_t>(kBatchSize) * static_cast<std::size_t>(kMsIdEntryStride));

    std::uint32_t objCount = 0;

    for (std::uint32_t batchStart = 0; batchStart < capacity; batchStart += kBatchSize)
    {
        const std::uint32_t batchEnd = (batchStart + kBatchSize < capacity) ? (batchStart + kBatchSize) : capacity;
        const std::uint32_t batchCount = batchEnd - batchStart;
        const std::size_t batchSize = static_cast<std::size_t>(batchCount) * static_cast<std::size_t>(kMsIdEntryStride);

        const std::uintptr_t batchAddr = entriesBase + static_cast<std::uintptr_t>(batchStart) * static_cast<std::uintptr_t>(kMsIdEntryStride);
        if (!mem.Read(batchAddr, batch.data(), batchSize))
        {
            continue;
        }

        for (std::uint32_t i = 0; i < batchCount; ++i)
        {
            const std::size_t entryOff = static_cast<std::size_t>(i) * static_cast<std::size_t>(kMsIdEntryStride);

            std::uint32_t hashMask = 0;
            std::memcpy(&hashMask, batch.data() + entryOff + static_cast<std::size_t>(kMsIdEntryHashMask), sizeof(hashMask));
            
            if (hashMask == 0xFFFFFFFFu || hashMask == 0xFFFFFFFEu)
            {
                continue;
            }

            std::uintptr_t obj = 0;
            std::memcpy(&obj, batch.data() + entryOff + static_cast<std::size_t>(kMsIdEntryObject), sizeof(obj));

            if (ValidateUnityObject(mem, obj, gomOff))
            {
                ++objCount;
            }
        }

        if (objCount >= expectedCount)
        {
            break;
        }
    }

    return objCount;
}

inline bool FindMsIdToPointerSlotVaByScan(
    const IMemoryAccessor& mem,
    const ModuleInfo& unityPlayer,
    const GomOffsets& gomOff,
    std::uintptr_t& outMsIdToPointerSlotVa,
    std::uint32_t* outBestObjCount = nullptr)
{
    outMsIdToPointerSlotVa = 0;
    if (outBestObjCount)
    {
        *outBestObjCount = 0;
    }

    std::uint32_t sizeOfImage = 0;
    std::vector<ModuleSection> sections;
    if (!ReadModuleSections(mem, unityPlayer.base, sizeOfImage, sections))
    {
        return false;
    }

    std::uintptr_t bestAddr = 0;
    std::uint32_t bestObjCount = 0;

    for (const auto& s : sections)
    {
        if (!(s.name == ".data" || s.name == ".rdata"))
        {
            continue;
        }
        if (s.size == 0)
        {
            continue;
        }

        std::vector<std::uint8_t> buf;
        buf.resize(s.size);

        const std::uintptr_t secVa = unityPlayer.base + static_cast<std::uintptr_t>(s.rva);
        if (!mem.Read(secVa, buf.data(), buf.size()))
        {
            continue;
        }

        for (std::size_t i = 0; i + sizeof(std::uintptr_t) <= buf.size(); i += sizeof(std::uintptr_t))
        {
            std::uintptr_t ptr = 0;
            std::memcpy(&ptr, buf.data() + i, sizeof(ptr));
            if (!IsCanonicalUserPtr(ptr))
            {
                continue;
            }

            std::uint8_t baseData[0x10] = {};
            if (!mem.Read(ptr, baseData, sizeof(baseData)))
            {
                continue;
            }

            std::uintptr_t entriesBase = 0;
            std::memcpy(&entriesBase, baseData + static_cast<std::size_t>(kMsIdSetEntriesBase), sizeof(entriesBase));
            if (!IsCanonicalUserPtr(entriesBase))
            {
                continue;
            }
            if (!IsReadableByte(mem, entriesBase))
            {
                continue;
            }

            std::uint32_t capacity = 0;
            std::memcpy(&capacity, baseData + static_cast<std::size_t>(kMsIdSetCapacity), sizeof(capacity));
            
            std::uint32_t count = 0;
            std::memcpy(&count, baseData + static_cast<std::size_t>(kMsIdSetCount), sizeof(count));
            if (count == 0 || count > 5000000)
            {
                continue;
            }
            if (capacity == 0 || capacity > 20000000)
            {
                continue;
            }

            const std::uint32_t objCount = CountUnityObjectsInMsIdEntriesPool(mem, entriesBase, capacity, count, gomOff);
            if (objCount == 0)
            {
                continue;
            }

            const std::uintptr_t addr = secVa + static_cast<std::uintptr_t>(i);
            if (objCount > bestObjCount)
            {
                bestObjCount = objCount;
                bestAddr = addr;
            }
        }
    }

    if (!bestAddr)
    {
        return false;
    }

    outMsIdToPointerSlotVa = bestAddr;
    if (outBestObjCount)
    {
        *outBestObjCount = bestObjCount;
    }
    return true;
}

} // namespace er2
