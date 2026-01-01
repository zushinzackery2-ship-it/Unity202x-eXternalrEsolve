#pragma once

#include <cstdint>
#include <string>

#include "../../util/ascii.hpp"

namespace er2
{

enum class ManagedBackend : std::uint8_t
{
    Mono = 1,
    Il2Cpp = 2,
};

enum class Feature : std::uint8_t
{
    MsIdToPointer = 1,
    ManagedObjectChain = 2,
    ManagedClassName = 3,
    TypeManager = 4,
    Il2CppMetadata = 5,
};

inline bool Supports(ManagedBackend backend, Feature feature)
{
    switch (feature)
    {
    case Feature::Il2CppMetadata:
        return backend == ManagedBackend::Il2Cpp;
    default:
        return true;
    }
}

inline const char* ToString(ManagedBackend backend)
{
    switch (backend)
    {
    case ManagedBackend::Mono:
        return "Mono";
    case ManagedBackend::Il2Cpp:
        return "IL2CPP";
    default:
        return "Unknown";
    }
}

inline bool TryParseManagedBackend(const std::string& s, ManagedBackend& out)
{
    const std::string v = er2::ToLowerAscii(s);
    if (v == "mono")
    {
        out = ManagedBackend::Mono;
        return true;
    }
    if (v == "il2cpp" || v == "il2" || v == "il")
    {
        out = ManagedBackend::Il2Cpp;
        return true;
    }
    return false;
}

} // namespace er2
