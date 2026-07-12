#pragma once

#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <cstdint>
#include <string>

namespace er2
{

struct RegistrationInitResult;

bool Collect(
    std::uintptr_t moduleBase,
    std::uint32_t moduleSize,
    const uint8_t* metaBytes,
    size_t metaSize,
    uintptr_t metaBase,
    CollectedData& out,
    std::string& error,
    RegistrationInitResult* registrationOut = nullptr);

} // namespace er2
