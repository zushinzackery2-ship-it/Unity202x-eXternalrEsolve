#pragma once

#include <cstdint>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../../mem/memory_read.hpp"
#include "../gom/gom_offsets.hpp"
#include "../gom/gom_search.hpp"
#include "../object/native/native_component.hpp"

namespace er2
{

/// <summary>


/// </summary>
struct CameraOffsets
{
    std::uint32_t view_proj_matrix = 0x100;  
};

inline bool GetCameraViewProjMatrix(const IMemoryAccessor& mem, std::uintptr_t nativeCamera, const CameraOffsets& off, float outMatrix[16])
{
    if (!nativeCamera || !outMatrix)
    {
        return false;
    }

    return mem.Read(nativeCamera + off.view_proj_matrix, outMatrix, 64);
}

inline bool GetCameraMatrix(const IMemoryAccessor& mem, std::uintptr_t nativeCamera, const CameraOffsets& off, glm::mat4& outViewProj)
{
    float data[16] = {};
    if (!GetCameraViewProjMatrix(mem, nativeCamera, off, data))
    {
        return false;
    }

    outViewProj = glm::make_mat4(data);
    return true;
}

inline bool FindMainCamera(
    const IMemoryAccessor& mem,
    std::uintptr_t gomGlobalSlot,
    const GomOffsets& gomOff,
    std::int32_t mainCameraTag,
    std::int32_t cameraTypeId,
    std::uintptr_t& outNativeCamera)
{
    outNativeCamera = 0;

    const std::uintptr_t mainGo = FindGameObjectThroughTag(mem, gomGlobalSlot, gomOff, mainCameraTag);
    if (!mainGo)
    {
        return false;
    }

    const std::uintptr_t camNative = GetComponentThroughTypeId(mem, gomOff, mainGo, cameraTypeId);
    if (!camNative)
    {
        return false;
    }

    if (!IsComponentEnabled(mem, camNative, gomOff))
    {
        return false;
    }

    outNativeCamera = camNative;
    return true;
}

} // namespace er2
