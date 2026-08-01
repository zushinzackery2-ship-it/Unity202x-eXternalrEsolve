#include "sidecar_writer_internal.hpp"

#include <cctype>
#include <cstdio>
#include <set>

namespace er2::SidecarDetail
{

std::string EscapeJsonString(const std::string& value)
{
    std::string result;
    result.reserve(value.size() + 16);
    for (const char c : value)
    {
        switch (c)
        {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                char buffer[8] = {};
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(c));
                result += buffer;
            }
            else
            {
                result += c;
            }
            break;
        }
    }
    return result;
}

std::string EscapeCppName(const std::string& name)
{
    static const std::set<std::string> keywords = {
        "klass", "monitor", "register", "_cs", "auto", "friend", "template",
        "new", "delete", "this", "class", "struct", "enum", "union",
        "public", "private", "protected", "virtual", "override", "static",
        "const", "volatile", "extern", "inline", "if", "else", "while",
        "for", "do", "switch", "case", "default", "break", "continue",
        "return", "goto", "try", "catch", "throw", "namespace", "using",
        "typedef", "typename", "sizeof", "alignof", "nullptr", "true", "false",
        "and", "or", "not", "xor", "bitand", "bitor", "compl", "and_eq",
        "or_eq", "not_eq", "xor_eq", "near", "far", "far16"
    };
    std::string result = name;
    for (char& c : result)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0 && c != '_')
        {
            c = '_';
        }
    }
    if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0])) != 0)
    {
        result = "_" + result;
    }
    if (keywords.count(result) != 0)
    {
        result = "_" + result + "_";
    }
    return result;
}

std::string ToCppType(const std::string& csharpType, bool isValueType)
{
    if (csharpType == "void") return "void";
    if (csharpType == "bool" || csharpType == "Boolean") return "bool";
    if (csharpType == "byte" || csharpType == "Byte") return "uint8_t";
    if (csharpType == "sbyte" || csharpType == "SByte") return "int8_t";
    if (csharpType == "char" || csharpType == "Char") return "uint16_t";
    if (csharpType == "short" || csharpType == "Int16") return "int16_t";
    if (csharpType == "ushort" || csharpType == "UInt16") return "uint16_t";
    if (csharpType == "int" || csharpType == "Int32") return "int32_t";
    if (csharpType == "uint" || csharpType == "UInt32") return "uint32_t";
    if (csharpType == "long" || csharpType == "Int64") return "int64_t";
    if (csharpType == "ulong" || csharpType == "UInt64") return "uint64_t";
    if (csharpType == "float" || csharpType == "Single") return "float";
    if (csharpType == "double" || csharpType == "Double") return "double";
    if (csharpType == "string" || csharpType == "String") return "System_String_o*";
    if (csharpType == "IntPtr" || csharpType == "UIntPtr") return "intptr_t";
    if (csharpType == "object" || csharpType == "Object") return "Il2CppObject*";
    if (csharpType.find('[') != std::string::npos ||
        csharpType.find('*') != std::string::npos ||
        csharpType.find('<') != std::string::npos)
    {
        return "Il2CppObject*";
    }
    return EscapeCppName(csharpType) + (isValueType ? "_o" : "_o*");
}

} // namespace er2::SidecarDetail
