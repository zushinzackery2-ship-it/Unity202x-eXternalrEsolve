#pragma once

#include <cstdint>

namespace er2
{

struct GameObjectEntry
{
    std::uintptr_t node = 0;
    std::uintptr_t nativeObject = 0;
    std::uintptr_t managedObject = 0;
};

struct ComponentEntry
{
    std::uintptr_t nativeComponent = 0;
    std::uintptr_t managedComponent = 0;
};

} // namespace er2
