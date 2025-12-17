#pragma once

#include <cstdint>
#include <vector>

#include "../Globals/GOMGlobal.hpp"
#include "WalkerTypes.hpp"

namespace UnityExternal
{

class GOMWalker
{
public:
    GOMWalker(const IMemoryAccessor& mem, RuntimeKind runtime)
        : mem_(mem), runtime_(runtime) {}

    bool EnumerateGameObjects(std::vector<GameObjectEntry>& out) const;
    bool EnumerateComponents(std::vector<ComponentEntry>& out) const;

    RuntimeKind GetRuntime() const { return runtime_; }
    const IMemoryAccessor& Accessor() const { return mem_; }

private:
    const IMemoryAccessor& mem_;
    RuntimeKind runtime_;
    std::uintptr_t gomGlobal_ = 0;

    bool EnumerateGameObjectsImpl(std::uintptr_t managerAddress, std::vector<GameObjectEntry>& out) const;
    bool EnumerateComponentsImpl(std::uintptr_t managerAddress, std::vector<ComponentEntry>& out) const;

    friend void SetGOMGlobal(std::uintptr_t);
};

} // namespace UnityExternal
