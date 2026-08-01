#pragma once

#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <cstdint>
#include <string>

namespace er2
{

class SidecarWriter
{
public:
    static bool WriteAll(const std::string& outputDir, const CollectedData& data, std::uintptr_t moduleBase);

private:
    static bool WriteDumpCs(const std::string& path, const CollectedData& data, std::uintptr_t moduleBase);
    static bool WriteIl2CppHeader(const std::string& path, const CollectedData& data);
    static bool WriteScriptJson(const std::string& path, const CollectedData& data, std::uintptr_t moduleBase);
    static bool WriteStringLiteralJson(const std::string& path, const CollectedData& data, std::uintptr_t moduleBase);
};

} // namespace er2
