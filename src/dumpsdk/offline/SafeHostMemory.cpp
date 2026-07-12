#include <er2/unity2/dumpsdk/offline/SafeHostMemory.h>

#include <Psapi.h>
#include <algorithm>
#include <cstring>

namespace er2
{

namespace
{

bool RegionAllowsRead(DWORD protect)
{
    switch (protect & 0xFF)
    {
    case PAGE_READONLY:
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

bool AddressRangeWraps(uintptr_t address, size_t size)
{
    return size != 0 && address + size < address;
}

int SehCopy(uintptr_t address, void* destination, size_t size)
{
    __try
    {
        std::memcpy(destination, reinterpret_cast<const void*>(address), size);
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

} // namespace

bool HostMemoryIsReadable(uintptr_t address, size_t size)
{
    if (address == 0 || size == 0 || AddressRangeWraps(address, size))
    {
        return false;
    }

    size_t remaining = size;
    uintptr_t cursor = address;
    while (remaining > 0)
    {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &info, sizeof(info)) == 0)
        {
            return false;
        }
        if (info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0 || !RegionAllowsRead(info.Protect))
        {
            return false;
        }

        const uintptr_t regionStart = reinterpret_cast<uintptr_t>(info.BaseAddress);
        const uintptr_t regionEnd = regionStart + info.RegionSize;
        if (cursor < regionStart || cursor >= regionEnd)
        {
            return false;
        }

        const size_t chunk = static_cast<size_t>((std::min)(regionEnd - cursor, static_cast<uintptr_t>(remaining)));
        remaining -= chunk;
        cursor += chunk;
    }
    return true;
}

bool HostMemoryTryRead(uintptr_t address, void* destination, size_t size)
{
    if (destination == nullptr || size == 0)
    {
        return false;
    }
    if (!HostMemoryIsReadable(address, size))
    {
        return false;
    }
    return SehCopy(address, destination, size) != 0;
}

bool HostMemoryTryReadU32(uintptr_t address, uint32_t& value)
{
    return HostMemoryTryRead(address, &value, sizeof(value));
}

bool HostMemoryTryReadU64(uintptr_t address, uint64_t& value)
{
    return HostMemoryTryRead(address, &value, sizeof(value));
}

bool HostMemoryTryReadI32(uintptr_t address, int32_t& value)
{
    return HostMemoryTryRead(address, &value, sizeof(value));
}

bool HostMemorySehTryRead(uintptr_t address, void* destination, size_t size)
{
    if (destination == nullptr || size == 0 || address == 0)
    {
        return false;
    }
    return SehCopy(address, destination, size) != 0;
}

bool HostMemorySehTryReadU32(uintptr_t address, uint32_t& value)
{
    return HostMemorySehTryRead(address, &value, sizeof(value));
}

bool HostMemoryTrySnapshotRange(
    uintptr_t moduleBase,
    size_t moduleSize,
    std::vector<uint8_t>& outImage,
    std::string& error)
{
    outImage.clear();
    error.clear();

    if (moduleBase == 0 || moduleSize == 0)
    {
        error = "invalid module image size";
        return false;
    }

    outImage.assign(moduleSize, 0);

    size_t offset = 0;
    while (offset < moduleSize)
    {
        const uintptr_t absAddr = moduleBase + offset;
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(absAddr), &info, sizeof(info)) == 0)
        {
            offset += 0x1000;
            continue;
        }

        const uintptr_t regionStart = reinterpret_cast<uintptr_t>(info.BaseAddress);
        const uintptr_t regionEnd = regionStart + info.RegionSize;
        const size_t chunk = static_cast<size_t>((std::min)(regionEnd - absAddr, static_cast<uintptr_t>(moduleSize - offset)));
        if (chunk == 0)
        {
            offset += 0x1000;
            continue;
        }

        if (info.State == MEM_COMMIT && (info.Protect & PAGE_GUARD) == 0 && RegionAllowsRead(info.Protect))
        {
            SehCopy(absAddr, outImage.data() + offset, chunk);
        }

        offset += chunk;
    }

    return true;
}

bool HostMemoryTrySnapshotModule(
    HMODULE module,
    std::vector<uint8_t>& outImage,
    uintptr_t& outBase,
    size_t& outSize,
    std::string& error)
{
    outImage.clear();
    outBase = 0;
    outSize = 0;
    error.clear();

    if (module == nullptr)
    {
        error = "module is null";
        return false;
    }

    MODULEINFO modInfo{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &modInfo, sizeof(modInfo)))
    {
        error = "GetModuleInformation failed";
        return false;
    }

    outBase = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    outSize = modInfo.SizeOfImage;
    return HostMemoryTrySnapshotRange(outBase, outSize, outImage, error);
}

} // namespace er2
