#pragma once

#include <optional>

#include "context.hpp"

#include "../camera/camera.hpp"
#include "component.hpp"

namespace er2
{

inline std::uintptr_t FindMainCameraNative(std::int32_t mainCameraTag, std::int32_t cameraTypeId)
{
    if (!IsInited() || !g_ctx.gomGlobalSlotVa)
    {
        return 0;
    }

    std::uintptr_t nativeCam = 0;
    if (!er2::FindMainCamera(Mem(), g_ctx.gomGlobalSlotVa, g_ctx.gomOff, mainCameraTag, cameraTypeId, nativeCam))
    {
        return 0;
    }

    return nativeCam;
}

inline std::uintptr_t FindMainCamera()
{
    if (!IsInited() || !g_ctx.gomGlobalSlotVa)
    {
        return 0;
    }

    const std::int32_t mainCameraTag = 5;
    const std::uintptr_t mainGo = er2::FindGameObjectThroughTag(Mem(), g_ctx.gomGlobalSlotVa, g_ctx.gomOff, mainCameraTag);
    if (!mainGo)
    {
        return 0;
    }

    const std::uintptr_t nativeCam = er2::GetCameraComponent(mainGo);
    if (!nativeCam)
    {
        return 0;
    }
    return nativeCam;
}

inline bool GetCameraMatrix(std::uintptr_t nativeCamera, glm::mat4& outViewProj)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::GetCameraMatrix(Mem(), nativeCamera, g_ctx.camOff, outViewProj);
}

inline std::optional<glm::mat4> GetCameraMatrix(std::uintptr_t nativeCamera)
{
    glm::mat4 viewProj(1.0f);
    if (!GetCameraMatrix(nativeCamera, viewProj))
    {
        return std::nullopt;
    }
    return viewProj;
}

} // namespace er2
