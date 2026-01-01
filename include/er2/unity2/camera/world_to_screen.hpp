#pragma once

#include "glm/glm.hpp"

namespace er2
{

struct ScreenRect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct WorldToScreenResult
{
    bool visible = false;
    float x = 0.0f;
    float y = 0.0f;
    float depth = 0.0f;
};

inline WorldToScreenResult WorldToScreenPoint(const glm::mat4& viewProj, const ScreenRect& screen, const glm::vec3& worldPos)
{
    WorldToScreenResult out;

    const glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.001f)
    {
        return out;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    const float sx = (ndc.x + 1.0f) * 0.5f * screen.width + screen.x;
    const float sy = (1.0f - ndc.y) * 0.5f * screen.height + screen.y;

    out.x = sx;
    out.y = sy;
    out.depth = clip.w;
    out.visible = (ndc.x >= -1.0f && ndc.x <= 1.0f && ndc.y >= -1.0f && ndc.y <= 1.0f);
    return out;
}

inline WorldToScreenResult WorldToScreenPointFull(const glm::mat4& viewProj, const glm::vec3& camPos, const glm::vec3& camForward, const ScreenRect& screen, const glm::vec3& worldPos)
{
    WorldToScreenResult out;

    const glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.001f)
    {
        return out;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    const float sx = (ndc.x + 1.0f) * 0.5f * screen.width + screen.x;
    const float sy = (1.0f - ndc.y) * 0.5f * screen.height + screen.y;

    const glm::vec3 delta = worldPos - camPos;
    const float depth = glm::dot(delta, camForward);

    out.x = sx;
    out.y = sy;
    out.depth = depth;
    out.visible = (depth > 0.0f);
    return out;
}

} // namespace er2
