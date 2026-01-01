#pragma once

#include <optional>

#include "context.hpp"

#include "../object/native/native_game_object_name.hpp"
#include "../object/native/native_scriptable_object_name.hpp"
#include "../object/native/native_game_object.hpp"
#include "../object/managed/managed_object.hpp"

namespace er2
{

inline bool GetGameObjectName(std::uintptr_t gameObject, std::string& outName)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::ReadNativeGameObjectName(Mem(), gameObject, g_ctx.gomOff, outName);
}

inline std::optional<std::string> GetGameObjectName(std::uintptr_t gameObject)
{
    std::string name;
    if (!GetGameObjectName(gameObject, name))
    {
        return std::nullopt;
    }
    return name;
}

inline bool GetScriptableObjectName(std::uintptr_t scriptableObject, std::string& outName)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::ReadScriptableObjectName(Mem(), scriptableObject, g_ctx.off, outName);
}

inline std::optional<std::string> GetScriptableObjectName(std::uintptr_t scriptableObject)
{
    std::string name;
    if (!GetScriptableObjectName(scriptableObject, name))
    {
        return std::nullopt;
    }
    return name;
}

inline bool GetManagedObjectTypeInfo(std::uintptr_t managed, TypeInfo& out)
{
    if (!IsInited())
    {
        return false;
    }

    return er2::ReadManagedObjectTypeInfo(Mem(), managed, g_ctx.off, out);
}

inline std::optional<TypeInfo> GetManagedObjectTypeInfo(std::uintptr_t managed)
{
    TypeInfo info;
    if (!GetManagedObjectTypeInfo(managed, info))
    {
        return std::nullopt;
    }
    return info;
}

} // namespace er2
