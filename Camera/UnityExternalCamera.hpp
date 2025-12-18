#pragma once

#include <cstdint>

#include "../Core/UnityExternalMemoryConfig.hpp"
#include "../GameObjectManager/GOMpUnit/GOM.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace UnityExternal
{

inline bool IsComponentEnabled(std::uintptr_t nativeComponent)
{
    if (!nativeComponent) return false;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return false;
    bool enabled = false;
    if (!GetComponentEnabled(*acc, nativeComponent, enabled)) return false;
    return enabled;
}

inline glm::mat4 GetCameraMatrix(std::uintptr_t nativeCamera)
{
    glm::mat4 matrix(1.0f);
    if (!nativeCamera) return matrix;
    const IMemoryAccessor* acc = GetGlobalMemoryAccessor();
    if (!acc) return matrix;
    float data[16] = {};
    if (GetCameraViewProjMatrix(*acc, nativeCamera, data))
    {
        matrix = glm::make_mat4(data);
    }
    return matrix;
}

inline std::uintptr_t FindMainCamera()
{
    std::uintptr_t mainGo = FindGameObjectThroughTag(5);
    if (!mainGo) return 0;
    std::uintptr_t camNative = GetComponentThroughTypeName(mainGo, "Camera");
    if (!camNative || !IsComponentEnabled(camNative)) return 0;
    return camNative;
}

} // namespace UnityExternal
