#pragma once

#include <er2/mem/memory_accessor.hpp>
#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <Windows.h>
#include <cstdint>
#include <string>

namespace er2
{

/// In-process offline collect entry used by DumpSdkRunInProcess.
/// `mem` is used for metadata pointer scoring / region reads.
/// Module image snapshot uses SEH host reads at moduleBase..moduleBase+moduleSize.
bool DumpIl2CppOfflineCollect(
    const IMemoryAccessor& mem,
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const std::string& outDir,
    CollectedData& data,
    std::string& error);

/// Convenience wrapper: resolves MODULEINFO from HMODULE then calls DumpIl2CppOfflineCollect.
bool DumpIl2CppOfflineCollectFromModule(
    HMODULE runtimeModule,
    const std::string& outDir,
    CollectedData& data,
    std::string& error);

} // namespace er2
