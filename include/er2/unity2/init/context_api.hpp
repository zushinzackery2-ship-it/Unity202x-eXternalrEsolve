#pragma once

#include <cstdint>
#include <optional>

#include "context_types.hpp"
#include "context_backend.hpp"
#include "../../mem/memory_read.hpp"

namespace er2
{

inline bool IsInited()
{
    return EnsureContextBackend() && g_ctx.backend->IsAttached() && g_ctx.pid != 0 && g_ctx.unityPlayer.base != 0;
}

inline std::uint32_t Pid()
{
    return g_ctx.pid;
}

inline ManagedBackend Runtime()
{
    return g_ctx.runtime;
}

inline std::uintptr_t UnityPlayerBase()
{
    return g_ctx.unityPlayer.base;
}

inline std::uintptr_t GomGlobalSlotVa()
{
    return g_ctx.gomGlobalSlotVa;
}

inline std::uintptr_t MsIdToPointerSlotVa()
{
    return g_ctx.msIdToPointerSlotVa;
}

inline const Offsets& Off()
{
    return g_ctx.off;
}

inline const GomOffsets& GomOff()
{
    return g_ctx.gomOff;
}

inline const CameraOffsets& CamOff()
{
    return g_ctx.camOff;
}

inline const TransformOffsets& TransformOff()
{
    return g_ctx.transformOff;
}

inline const IMemoryAccessor& Mem()
{
    if (!EnsureContextBackend())
    {
        return GetNullMemoryAccessor();
    }

    const IMemoryAccessor* accessor = g_ctx.backend->GetMemoryAccessor();
    if (!accessor)
    {
        return GetNullMemoryAccessor();
    }

    return *accessor;
}

inline bool ReadPtr(std::uintptr_t address, std::uintptr_t& out)
{
    if (!IsInited())
    {
        out = 0;
        return false;
    }

    return er2::ReadPtr(Mem(), address, out);
}

inline std::optional<std::uintptr_t> ReadPtr(std::uintptr_t address)
{
    std::uintptr_t out = 0;
    if (!ReadPtr(address, out))
    {
        return std::nullopt;
    }
    return out;
}

template <typename T>
inline bool ReadValue(std::uintptr_t address, T& out)
{
    if (!IsInited())
    {
        out = T{};
        return false;
    }

    return er2::ReadValue(Mem(), address, out);
}

template <typename T>
inline std::optional<T> ReadValue(std::uintptr_t address)
{
    T out{};
    if (!ReadValue(address, out))
    {
        return std::nullopt;
    }
    return out;
}

} // namespace er2
