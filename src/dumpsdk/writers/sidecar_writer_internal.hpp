#pragma once

#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <string>

namespace er2::SidecarDetail
{

std::string EscapeJsonString(const std::string& value);
std::string EscapeCppName(const std::string& name);
std::string ToCppType(const std::string& csharpType, bool isValueType);
std::string BuildTypeDeclaration(const CollectedType& type);
std::string BuildBaseClause(const CollectedType& type);
std::string BuildFieldModifiers(const CollectedField& field);
std::string BuildMethodModifiers(const CollectedMethod& method);
std::string BuildParamText(const CollectedParam& parameter);
std::string BuildMethodSignature(
    const CollectedMethod& method,
    const std::string& className,
    bool isStatic);
std::string BuildTypeSignature(const CollectedMethod& method);

} // namespace er2::SidecarDetail
