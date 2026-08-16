#include <er2/unity2/dumpsdk/writers/dummy_dll_generator.hpp>
#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/dump_progress.hpp>
#include <er2/unity2/dumpsdk/writers/cli/UnityCliWriter.h>

#include <cctype>
#include <filesystem>

namespace
{

bool EndsWithIgnoreCase(const std::string& value, const char* suffix)
{
    const std::size_t suffixLength = std::char_traits<char>::length(suffix);
    if (value.size() < suffixLength)
    {
        return false;
    }

    for (std::size_t index = 0; index < suffixLength; ++index)
    {
        const unsigned char left = static_cast<unsigned char>(
            value[value.size() - suffixLength + index]);
        const unsigned char right = static_cast<unsigned char>(suffix[index]);
        if (std::tolower(left) != std::tolower(right))
        {
            return false;
        }
    }
    return true;
}

std::string AssemblyFileStem(std::string name)
{
    if (EndsWithIgnoreCase(name, ".dll"))
    {
        name.resize(name.size() - 4);
    }
    if (name.empty())
    {
        name = "UnityDummyAssembly";
    }
    for (char& character : name)
    {
        if (character == '<' || character == '>' || character == ':' || character == '*'
            || character == '?' || character == '|' || character == '"'
            || character == '/' || character == '\\')
        {
            character = '_';
        }
    }
    return name;
}

} // namespace

namespace er2
{

bool DummyDllGenerator::Generate(const std::string& outputDir, const CollectedData& data)
{
    DumpSdkProgressScope progress("Generate DummyDll", data.assemblies.size());
    const std::string dllDir = outputDir + "\\DummyDll";
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(dllDir), ec);
    if (ec)
    {
        DumpSdkLog(DumpSdkLogLevel::Error, "[DummyDll] create_directories failed: " + dllDir);
        return false;
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        "[DummyDll] Generate begin, assemblies=" + std::to_string(data.assemblies.size()));

    size_t assemblyIndex = 0;
    for (const auto& asm_ : data.assemblies)
    {
        if (!WriteAssembly(dllDir, asm_))
        {
            DumpSdkLog(DumpSdkLogLevel::Error, "[DummyDll] WriteAssembly failed");
            progress.Fail(asm_.name);
            return false;
        }
        progress.Update(++assemblyIndex, asm_.name);
    }

    DumpSdkLog(DumpSdkLogLevel::Info, "[DummyDll] Generate ok");
    progress.Complete();
    return true;
}

bool DummyDllGenerator::WriteAssembly(const std::string& dir, const CollectedAssembly& asm_)
{
    const std::string safeName = AssemblyFileStem(
        asm_.name.empty() ? asm_.fileName : asm_.name);
    return UnityCli::WriteAssembly(dir + "\\" + safeName + ".dll", asm_);
}

bool DummyDllGenerator::WriteMinimalDotNetDll(const std::string& path, const CollectedAssembly& asm_)
{
    return UnityCli::WriteAssembly(path, asm_);
}

} // namespace er2
