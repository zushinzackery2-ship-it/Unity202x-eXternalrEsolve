#pragma once

#include <cstdint>

namespace UnityExternal
{

struct GameObjectEntry
{
    std::uintptr_t node;
    std::uintptr_t nativeObject;
    std::uintptr_t managedObject;
};

struct ComponentEntry
{
    std::uintptr_t nativeComponent;
    std::uintptr_t managedComponent;
};

} // namespace UnityExternal
