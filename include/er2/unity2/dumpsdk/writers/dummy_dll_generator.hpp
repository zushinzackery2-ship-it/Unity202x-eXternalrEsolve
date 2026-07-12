#pragma once

#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <string>

namespace er2
{

class DummyDllGenerator
{
public:
    static bool Generate(const std::string& outputDir, const CollectedData& data);

private:
    static bool WriteAssembly(const std::string& dir, const CollectedAssembly& asm_);
    static bool WriteMinimalDotNetDll(const std::string& path, const CollectedAssembly& asm_);
};

} // namespace er2
