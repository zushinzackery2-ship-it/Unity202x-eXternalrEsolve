#pragma once

#include "../../os/win/win_include.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>

#include "backend.hpp"
#include "../core/offsets.hpp"
#include "../gom/gom_scan.hpp"
#include "../object/managed/managed_backend.hpp"
#include "../object/native/native_object.hpp"
#include "../camera/camera.hpp"
#include "../transform/transform.hpp"

namespace er2
{

struct Context
{
    std::uint32_t pid = 0;
    HANDLE process = nullptr;
    ContextBackendPtr backend = CreateDefaultContextBackend();

    ManagedBackend runtime = ManagedBackend::Mono;

    ModuleInfo unityPlayer;
    UnityPlayerRange unityPlayerRange;
    ModuleInfo gameAssembly;

    Offsets off;
    GomOffsets gomOff;
    CameraOffsets camOff;
    TransformOffsets transformOff;

    std::uintptr_t gomGlobalSlotVa = 0;
    std::uint64_t gomGlobalSlotRva = 0;

    std::uintptr_t msIdToPointerSlotVa = 0;
    std::uint64_t msIdToPointerSlotRva = 0;

    std::uintptr_t typeInfoTable = 0;
    std::uint32_t typeInfoCount = 0;
    std::unordered_map<std::uint32_t, std::string> byvalToFullName;
    std::unordered_map<std::string, std::uint32_t> fullNameToByval;
};

inline Context g_ctx;

} // namespace er2
