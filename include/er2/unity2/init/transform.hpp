#pragma once

#include <optional>

#include "context.hpp"

#include "../transform/transform.hpp"

namespace er2
{

inline bool GetTransformWorldPosition(std::uintptr_t transformAddress, glm::vec3& outPos)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::GetTransformWorldPosition(Mem(), g_ctx.transformOff, transformAddress, outPos);
}

inline std::optional<glm::vec3> GetTransformWorldPosition(std::uintptr_t transformAddress)
{
    glm::vec3 pos(0.0f);
    if (!GetTransformWorldPosition(transformAddress, pos))
    {
        return std::nullopt;
    }
    return pos;
}

} // namespace er2
