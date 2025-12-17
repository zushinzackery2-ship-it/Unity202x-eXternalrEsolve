#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <cstring>
#include <cwchar>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <vector>

#include "../../Core/UnityExternalMemory.hpp"
#include "../../Core/UnityExternalMemoryConfig.hpp"
#include "../Resolver/WinAPIResolver.hpp"

namespace UnityExternal
{

inline constexpr const wchar_t* kCrAtcherDeviceName = L"\\\\.\\CrAtcher_mEm";

#ifndef CTL_CODE_LOCAL
#define CTL_CODE_LOCAL(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

inline constexpr DWORD IOCTL_RMCR3_READ = CTL_CODE_LOCAL(0x22, 0x801, 2, 0);
inline constexpr DWORD IOCTL_RMCR3_GETMODULE = CTL_CODE_LOCAL(0x22, 0x802, 2, 0);
inline constexpr DWORD IOCTL_RMCR3_WRITE = CTL_CODE_LOCAL(0x22, 0x805, 0, 0);
inline constexpr DWORD IOCTL_RMCR3_GET_MAINMODULE = CTL_CODE_LOCAL(0x22, 0x806, 2, 0);

inline constexpr DWORD IOCTL_RMCR3_READ_BATCH = CTL_CODE_LOCAL(0x22, 0x808, 2, 0);

struct CrAtcherDriverInput
{
    std::uint32_t flag;
    std::uint32_t pid;
    std::uint64_t address;
    std::uint64_t buffer;
    std::uint32_t size;
};

struct RMCR3_READ_DESC
{
    unsigned long long address;
    unsigned long size;
    unsigned long reserved;
};

struct RMCR3_READ_BATCH
{
    unsigned long pid;
    unsigned long count;
    unsigned long reserved0;
    unsigned long reserved1;
    RMCR3_READ_DESC descriptors[1];
};

inline HANDLE OpenCrAtcherDevice()
{
    HANDLE hDriver = CreateFileW(
        kCrAtcherDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (hDriver == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }
    return hDriver;
}

inline bool CrAtcherReadMemory(HANDLE hDriver, DWORD pid, std::uint64_t address, void* buffer, std::size_t size)
{
    if (!hDriver || !pid || !address || !buffer || size == 0)
    {
        return false;
    }

    CrAtcherDriverInput in{};
    in.flag = 0;
    in.pid = static_cast<std::uint32_t>(pid);
    in.address = address;
    in.buffer = 0;
    in.size = static_cast<std::uint32_t>(size);

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_RMCR3_READ,
        &in,
        static_cast<DWORD>(sizeof(in)),
        buffer,
        static_cast<DWORD>(size),
        &bytesReturned,
        nullptr);

    if (!ok)
    {
        return false;
    }
    return bytesReturned == size;
}

inline bool CrAtcherWriteMemory(HANDLE hDriver, DWORD pid, std::uint64_t address, const void* buffer, std::size_t size)
{
    if (!hDriver || !pid || !address || !buffer || size == 0)
    {
        return false;
    }

    CrAtcherDriverInput in{};
    in.flag = 0;
    in.pid = static_cast<std::uint32_t>(pid);
    in.address = address;
    in.buffer = 0;
    in.size = static_cast<std::uint32_t>(size);

    std::vector<std::uint8_t> payload;
    payload.resize(sizeof(in) + size);
    memcpy(payload.data(), &in, sizeof(in));
    memcpy(payload.data() + sizeof(in), buffer, size);

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_RMCR3_WRITE,
        payload.data(),
        static_cast<DWORD>(payload.size()),
        nullptr,
        0,
        &bytesReturned,
        nullptr);

    return ok != FALSE;
}

inline std::uint64_t CrAtcherGetMainModule(HANDLE hDriver, DWORD pid)
{
    if (!hDriver || !pid)
    {
        return 0;
    }

    CrAtcherDriverInput in{};
    in.flag = 0;
    in.pid = static_cast<std::uint32_t>(pid);

    std::uint64_t outBase = 0;
    DWORD bytesReturned = 0;

    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_RMCR3_GET_MAINMODULE,
        &in,
        static_cast<DWORD>(sizeof(in)),
        &outBase,
        static_cast<DWORD>(sizeof(outBase)),
        &bytesReturned,
        nullptr);

    if (!ok || bytesReturned != sizeof(outBase))
    {
        return 0;
    }
    return outBase;
}

inline std::uint64_t CrAtcherGetModule(HANDLE hDriver, DWORD pid, const wchar_t* moduleName)
{
    if (!hDriver || !pid)
    {
        return 0;
    }

    CrAtcherDriverInput in{};
    in.flag = 0;
    in.pid = static_cast<std::uint32_t>(pid);

    std::vector<std::uint8_t> payload;
    if (moduleName && moduleName[0] != L'\0')
    {
        const std::size_t nameBytes = (wcslen(moduleName) + 1) * sizeof(wchar_t);
        payload.resize(sizeof(in) + nameBytes);
        memcpy(payload.data(), &in, sizeof(in));
        memcpy(payload.data() + sizeof(in), moduleName, nameBytes);
    }
    else
    {
        payload.resize(sizeof(in));
        memcpy(payload.data(), &in, sizeof(in));
    }

    std::uint64_t outBase = 0;
    DWORD bytesReturned = 0;

    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_RMCR3_GETMODULE,
        payload.data(),
        static_cast<DWORD>(payload.size()),
        &outBase,
        static_cast<DWORD>(sizeof(outBase)),
        &bytesReturned,
        nullptr);

    if (!ok || bytesReturned != sizeof(outBase))
    {
        return 0;
    }
    return outBase;
}

inline bool CrAtcherReadBatch(
    HANDLE hDriver,
    DWORD pid,
    const RMCR3_READ_DESC* descs,
    std::uint32_t count,
    void* outBuffer,
    std::size_t outBufferSize)
    {

    if (!hDriver || !pid || !descs || count == 0 || !outBuffer || outBufferSize == 0)
    {
        return false;
    }

    const std::size_t inSize = sizeof(RMCR3_READ_BATCH) + (static_cast<std::size_t>(count) - 1) * sizeof(RMCR3_READ_DESC);
    std::vector<std::uint8_t> inBuf;
    inBuf.resize(inSize);

    RMCR3_READ_BATCH* batch = reinterpret_cast<RMCR3_READ_BATCH*>(inBuf.data());
    batch->pid = static_cast<unsigned long>(pid);
    batch->count = static_cast<unsigned long>(count);
    batch->reserved0 = 0;
    batch->reserved1 = 0;
    memcpy(batch->descriptors, descs, static_cast<std::size_t>(count) * sizeof(RMCR3_READ_DESC));

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_RMCR3_READ_BATCH,
        batch,
        static_cast<DWORD>(inBuf.size()),
        outBuffer,
        static_cast<DWORD>(outBufferSize),
        &bytesReturned,
        nullptr);

    return ok != FALSE && bytesReturned == outBufferSize;
}

inline std::atomic<HANDLE> g_crAtcherHandle{nullptr};

inline void UseCrAtcherModuleResolver(HANDLE hDriver)
{
    g_crAtcherHandle.store(hDriver, std::memory_order_release);
    SetModuleBaseResolver([](DWORD pid, const wchar_t* moduleName) -> std::uintptr_t {
        HANDLE h = g_crAtcherHandle.load(std::memory_order_acquire);
        if (!h || !pid)
        {
            return 0;
        }
        return static_cast<std::uintptr_t>(CrAtcherGetModule(h, pid, moduleName));
    });
}

struct DriverMemoryAccessor final : IMemoryAccessor
{
    HANDLE driver;
    DWORD pid;

    DriverMemoryAccessor(HANDLE hDriver, DWORD targetPid)
        : driver(hDriver), pid(targetPid) {}

    DriverMemoryAccessor(HANDLE hDriver, DWORD targetPid,
                         bool (*)(HANDLE, DWORD, std::uint64_t, void*, std::size_t),
                         bool (*)(HANDLE, DWORD, std::uint64_t, const void*, std::size_t))
        : driver(hDriver), pid(targetPid) {}

    bool Read(std::uintptr_t address, void* buffer, std::size_t size) const override {
        return CrAtcherReadMemory(driver, pid, static_cast<std::uint64_t>(address), buffer, size);
    }

    bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const override {
        return CrAtcherWriteMemory(driver, pid, static_cast<std::uint64_t>(address), buffer, size);
    }
};

} // namespace UnityExternal

#endif
