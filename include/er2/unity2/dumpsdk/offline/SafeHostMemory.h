#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace er2
{

bool HostMemoryIsReadable(uintptr_t address, size_t size);

bool HostMemoryTryRead(uintptr_t address, void* destination, size_t size);

bool HostMemoryTryReadU32(uintptr_t address, uint32_t& value);

bool HostMemoryTryReadU64(uintptr_t address, uint64_t& value);

bool HostMemoryTryReadI32(uintptr_t address, int32_t& value);

// Hot-path probe: SEH only, no VirtualQuery. Use after a region is known readable,
// or when rejecting millions of pointer candidates cheaply.
bool HostMemorySehTryRead(uintptr_t address, void* destination, size_t size);

bool HostMemorySehTryReadU32(uintptr_t address, uint32_t& value);

bool HostMemoryTrySnapshotRange(
    uintptr_t moduleBase,
    size_t moduleSize,
    std::vector<uint8_t>& outImage,
    std::string& error);

bool HostMemoryTrySnapshotModule(
    HMODULE module,
    std::vector<uint8_t>& outImage,
    uintptr_t& outBase,
    size_t& outSize,
    std::string& error);

} // namespace er2
