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

    static std::string BuildMethodSignature(
        const CollectedMethod& method,
        const std::string& className,
        const std::string& namespaceName,
        bool isStatic);
    static std::string BuildTypeSignature(const CollectedMethod& method);
    static std::string GetTypeKindStr(TypeKind kind, bool isAbstract, bool isSealed);

    static std::string GetFieldModifiers(const CollectedField& field);
    static std::string GetMethodModifiers(const CollectedMethod& method);
    static std::string EscapeJsonString(const std::string& s);
    static std::string EscapeCppName(const std::string& name);
    static std::string ToCppType(const std::string& csharpType, bool isValueType);
};

} // namespace er2
