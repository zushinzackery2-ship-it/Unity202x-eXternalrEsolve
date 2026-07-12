#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>
#include <er2/unity2/dumpsdk/dump_log.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <set>
#include <sstream>

namespace er2
{

bool SidecarWriter::WriteAll(const std::string& outputDir, const CollectedData& data, std::uintptr_t moduleBase)
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(outputDir), ec);

    DumpSdkLog(DumpSdkLogLevel::Info, "[Sidecar] WriteAll begin: " + outputDir);

    bool ok = true;
    ok &= WriteDumpCs(outputDir + "\\dump.cs", data, moduleBase);
    ok &= WriteIl2CppHeader(outputDir + "\\il2cpp.h", data);
    ok &= WriteScriptJson(outputDir + "\\script.json", data, moduleBase);
    ok &= WriteStringLiteralJson(outputDir + "\\stringliteral.json", data, moduleBase);

    if (ok)
    {
        DumpSdkLog(DumpSdkLogLevel::Info, "[Sidecar] WriteAll ok");
    }
    else
    {
        DumpSdkLog(DumpSdkLogLevel::Error, "[Sidecar] WriteAll failed");
    }
    return ok;
}

bool SidecarWriter::WriteDumpCs(const std::string& path, const CollectedData& data, std::uintptr_t moduleBase)
{
    std::ofstream io(path, std::ios::binary);
    if (!io)
    {
        return false;
    }

    std::stringstream str;

    for (const auto& asm_ : data.assemblies)
    {
        str << std::format("// Image {}: {} - {}", asm_.imageIndex, asm_.fileName, asm_.typeStartIndex) << "\n";
    }
    str << "\n";

    for (const auto& asm_ : data.assemblies)
    {
        for (const auto& type : asm_.types)
        {
            str << "\n";
            str << std::format("// Namespace: {}", type.namespaceName) << "\n";

            if (type.isSerializable)
            {
                str << "[Serializable]\n";
            }

            std::string kindStr = GetTypeKindStr(type.kind, type.isAbstract, type.isSealed);
            std::string accessStr = type.accessModifier.empty() ? "public " : type.accessModifier + " ";

            std::string extendsStr;
            if (!type.parentName.empty() && type.kind != TypeKind::Struct && type.kind != TypeKind::Enum)
            {
                extendsStr = " : " + type.parentName;
            }

            if (!type.interfaces.empty())
            {
                if (extendsStr.empty())
                {
                    extendsStr = " : ";
                }
                else
                {
                    extendsStr += ", ";
                }
                for (size_t i = 0; i < type.interfaces.size(); i++)
                {
                    if (i > 0)
                    {
                        extendsStr += ", ";
                    }
                    extendsStr += type.interfaces[i];
                }
            }

            str << std::format("{}{}{}{}", accessStr, kindStr, type.name, extendsStr);
            str << " // TypeDefIndex: " << type.index;
            str << "\n{\n";

            if (!type.fields.empty())
            {
                str << "\t// Fields\n";
                for (const auto& field : type.fields)
                {
                    std::string mod = GetFieldModifiers(field);
                    std::string access = field.accessModifier.empty() ? "public " : field.accessModifier + " ";

                    str << std::format("\t{}{}{} {}", access, mod, field.typeName, field.name);

                    if (!field.defaultValue.empty())
                    {
                        str << " = " << field.defaultValue;
                    }

                    if (!field.isLiteral && field.offset != 0)
                    {
                        str << std::format("; // 0x{:X}", field.offset);
                    }
                    else
                    {
                        str << ";";
                    }

                    str << "\n";
                }
            }

            if (!type.properties.empty())
            {
                str << "\n\t// Properties\n";
                for (const auto& prop : type.properties)
                {
                    str << std::format("\t{}{} {} ", prop.modifiers.empty() ? "public " : prop.modifiers + " ", prop.typeName, prop.name);
                    str << "{ ";
                    if (prop.hasGetter)
                    {
                        str << "get; ";
                    }
                    if (prop.hasSetter)
                    {
                        str << "set; ";
                    }
                    str << "}\n";
                }
            }

            if (!type.methods.empty())
            {
                str << "\n\t// Methods\n";
                for (const auto& method : type.methods)
                {
                    if (method.address != 0)
                    {
                        uintptr_t rva = method.address - moduleBase;
                        str << std::format("\t// RVA: 0x{:X} Offset: 0x{:X} VA: 0x{:X}", rva, rva, method.address);
                        str << "\n";
                    }

                    std::string mod = GetMethodModifiers(method);
                    std::string methodAccess = method.accessModifier.empty() ? "public " : method.accessModifier + " ";

                    str << std::format("\t{}{}{} {}(", methodAccess, mod, method.returnType, method.name);

                    for (size_t i = 0; i < method.params.size(); i++)
                    {
                        if (i > 0)
                        {
                            str << ", ";
                        }
                        const auto& param = method.params[i];
                        if (param.isOut)
                        {
                            str << "out ";
                        }
                        else if (param.isIn)
                        {
                            str << "in ";
                        }
                        else if (param.isByRef)
                        {
                            str << "ref ";
                        }
                        str << param.typeName << " " << param.name;
                    }

                    if (method.isAbstract)
                    {
                        str << ");\n";
                    }
                    else
                    {
                        str << ") { }\n";
                    }

                    str << "\n";
                }
            }

            str << "}\n";
        }
    }

    io << str.str();
    io.close();
    return true;
}

bool SidecarWriter::WriteIl2CppHeader(const std::string& path, const CollectedData& data)
{
    std::ofstream io(path);
    if (!io)
    {
        return false;
    }

    std::stringstream str;

    str << "typedef void(*Il2CppMethodPointer)();\n";
    str << "struct MethodInfo;\n";
    str << "struct VirtualInvokeData\n{\n";
    str << "\tIl2CppMethodPointer methodPtr;\n";
    str << "\tconst MethodInfo* method;\n";
    str << "};\n\n";
    str << "struct Il2CppType\n{\n";
    str << "\tvoid* data;\n";
    str << "\tunsigned int bits;\n";
    str << "};\n\n";
    str << "struct Il2CppClass;\n";
    str << "struct Il2CppObject\n{\n";
    str << "\tIl2CppClass *klass;\n";
    str << "\tvoid *monitor;\n";
    str << "};\n\n";
    str << "union Il2CppRGCTXData\n{\n";
    str << "\tvoid* rgctxDataDummy;\n";
    str << "\tconst MethodInfo* method;\n";
    str << "\tconst Il2CppType* type;\n";
    str << "\tIl2CppClass* klass;\n";
    str << "};\n\n";

    for (const auto& asm_ : data.assemblies)
    {
        for (const auto& type : asm_.types)
        {
            std::string safeName = EscapeCppName(type.name);

            str << std::format("// {}::{}", type.namespaceName, type.name) << "\n";

            str << std::format("struct {}_Fields\n{{\n", safeName);
            if (!type.parentName.empty() && type.kind != TypeKind::Struct && type.kind != TypeKind::Enum)
            {
                std::string parentSafe = EscapeCppName(type.parentName);
                str << std::format("\t{}_Fields _parent;\n", parentSafe);
            }
            for (const auto& field : type.fields)
            {
                if (!field.isStatic)
                {
                    std::string cppType = ToCppType(field.typeName, type.kind == TypeKind::Struct || type.kind == TypeKind::Enum);
                    str << std::format("\t{} {}; // 0x{:X}", cppType, EscapeCppName(field.name), field.offset) << "\n";
                }
            }
            str << "};\n\n";

            if (type.isGeneric)
            {
                str << std::format("struct {}_RGCTXs\n{{\n", safeName);
                str << "\tIl2CppRGCTXData* rgctxData;\n";
                str << "};\n\n";
            }

            str << std::format("struct {}_VTable\n{{\n", safeName);
            str << "\tVirtualInvokeData vtable[32];\n";
            str << "};\n\n";

            str << std::format("struct {}_c\n{{\n", safeName);
            str << "\tIl2CppClass _klass;\n";
            str << "};\n\n";

            str << std::format("struct {}_o\n{{\n", safeName);
            if (type.kind != TypeKind::Struct && type.kind != TypeKind::Enum)
            {
                str << std::format("\t{}_c *klass;\n", safeName);
                str << "\tvoid *monitor;\n";
            }
            str << std::format("\t{}_Fields fields;\n", safeName);
            str << "};\n\n";

            bool hasStatic = false;
            for (const auto& field : type.fields)
            {
                if (field.isStatic)
                {
                    if (!hasStatic)
                    {
                        str << std::format("struct {}_StaticFields\n{{\n", safeName);
                        hasStatic = true;
                    }
                    std::string cppType = ToCppType(field.typeName, type.kind == TypeKind::Struct || type.kind == TypeKind::Enum);
                    str << std::format("\t{} {}; // 0x{:X}", cppType, EscapeCppName(field.name), field.offset) << "\n";
                }
            }
            if (hasStatic)
            {
                str << "};\n\n";
            }

            str << std::format("struct {}_array\n{{\n", safeName);
            str << "\tIl2CppObject obj;\n";
            str << "\tIl2CppArrayBounds *bounds;\n";
            str << "\tint max_length;\n";
            str << std::format("\t{}_Fields m_Items[65535];\n", safeName);
            str << "};\n\n";
        }
    }

    io << str.str();
    io.close();
    return true;
}

bool SidecarWriter::WriteScriptJson(const std::string& path, const CollectedData& data, std::uintptr_t moduleBase)
{
    std::ofstream io(path);
    if (!io)
    {
        return false;
    }

    io << "{\n";
    io << "  \"ScriptMethod\": [\n";

    bool first = true;
    for (const auto& asm_ : data.assemblies)
    {
        for (const auto& type : asm_.types)
        {
            for (const auto& method : type.methods)
            {
                if (method.address == 0)
                {
                    continue;
                }

                uintptr_t rva = method.address - moduleBase;
                std::string name = type.name + "$$" + method.name;
                std::string sig = BuildMethodSignature(method, type.name, type.namespaceName, method.isStatic);
                std::string typeSig = BuildTypeSignature(method);

                if (!first)
                {
                    io << ",\n";
                }
                first = false;

                io << "    {\n";
                io << std::format("      \"Address\": {},\n", rva);
                io << std::format("      \"Name\": \"{}\",\n", EscapeJsonString(name));
                io << std::format("      \"Signature\": \"{}\",\n", EscapeJsonString(sig));
                io << std::format("      \"TypeSignature\": \"{}\"\n", EscapeJsonString(typeSig));
                io << "    }";
            }
        }
    }

    io << "\n  ],\n";
    io << "  \"ScriptString\": [\n";

    first = true;
    for (const auto& s : data.strings)
    {
        if (s.address == 0)
        {
            continue;
        }
        uintptr_t rva = s.address - moduleBase;

        if (!first)
        {
            io << ",\n";
        }
        first = false;

        io << "    {\n";
        io << std::format("      \"Address\": {},\n", rva);
        io << std::format("      \"Value\": \"{}\"\n", EscapeJsonString(s.value));
        io << "    }";
    }

    io << "\n  ],\n";
    io << "  \"ScriptMetadata\": [\n";

    first = true;
    for (const auto& m : data.metadata)
    {
        if (m.address == 0)
        {
            continue;
        }
        uintptr_t rva = m.address - moduleBase;

        if (!first)
        {
            io << ",\n";
        }
        first = false;

        io << "    {\n";
        io << std::format("      \"Address\": {},\n", rva);
        io << std::format("      \"Name\": \"{}\",\n", EscapeJsonString(m.name));
        io << std::format("      \"Signature\": \"{}\"\n", EscapeJsonString(m.signature));
        io << "    }";
    }

    io << "\n  ],\n";
    io << "  \"ScriptMetadataMethod\": [\n";

    first = true;
    for (const auto& mr : data.methodRefs)
    {
        if (mr.address == 0)
        {
            continue;
        }
        uintptr_t rva = mr.address - moduleBase;
        uintptr_t methodRva = mr.methodAddress != 0 ? mr.methodAddress - moduleBase : 0;

        if (!first)
        {
            io << ",\n";
        }
        first = false;

        io << "    {\n";
        io << std::format("      \"Address\": {},\n", rva);
        io << std::format("      \"Name\": \"{}\",\n", EscapeJsonString(mr.name));
        io << std::format("      \"MethodAddress\": {}\n", methodRva);
        io << "    }";
    }

    io << "\n  ],\n";
    io << "  \"Addresses\": [\n";

    std::set<uintptr_t> sortedAddrs;
    for (const auto& asm_ : data.assemblies)
    {
        for (const auto& type : asm_.types)
        {
            for (const auto& method : type.methods)
            {
                if (method.address != 0)
                {
                    sortedAddrs.insert(method.address - moduleBase);
                }
            }
        }
    }
    for (const auto& s : data.strings)
    {
        if (s.address != 0)
        {
            sortedAddrs.insert(s.address - moduleBase);
        }
    }
    for (const auto& m : data.metadata)
    {
        if (m.address != 0)
        {
            sortedAddrs.insert(m.address - moduleBase);
        }
    }
    for (const auto& mr : data.methodRefs)
    {
        if (mr.address != 0)
        {
            sortedAddrs.insert(mr.address - moduleBase);
        }
    }

    first = true;
    for (uintptr_t addr : sortedAddrs)
    {
        if (!first)
        {
            io << ",\n";
        }
        first = false;
        io << std::format("    {}", addr);
    }

    io << "\n  ]\n";
    io << "}\n";

    io.close();
    return true;
}

bool SidecarWriter::WriteStringLiteralJson(const std::string& path, const CollectedData& data, std::uintptr_t moduleBase)
{
    (void)moduleBase;
    std::ofstream io(path);
    if (!io)
    {
        return false;
    }

    io << "[\n";

    bool first = true;
    for (const auto& s : data.strings)
    {
        if (!first)
        {
            io << ",\n";
        }
        first = false;

        io << "  {\n";
        io << std::format("    \"value\": \"{}\",\n", EscapeJsonString(s.value));
        io << std::format("    \"address\": \"0x{:X}\"", s.address);
        io << "\n  }";
    }

    io << "\n]\n";

    io.close();
    return true;
}

std::string SidecarWriter::BuildMethodSignature(
    const CollectedMethod& method,
    const std::string& className,
    const std::string& namespaceName,
    bool isStatic)
{
    (void)namespaceName;
    std::string retType = ToCppType(method.returnType, false);

    std::string sig = retType + " " + className + "$$" + method.name + " (";

    if (!isStatic)
    {
        sig += className + "_o* __this";
    }

    for (size_t i = 0; i < method.params.size(); i++)
    {
        if (i > 0 || !isStatic)
        {
            sig += ", ";
        }
        const auto& param = method.params[i];
        if (param.isOut)
        {
            sig += "out ";
        }
        else if (param.isIn)
        {
            sig += "in ";
        }
        else if (param.isByRef)
        {
            sig += "ref ";
        }
        sig += ToCppType(param.typeName, false) + " " + param.name;
    }

    if (!method.params.empty() || !isStatic)
    {
        sig += ", ";
    }
    sig += "const MethodInfo* method";

    sig += ");";
    return sig;
}

std::string SidecarWriter::BuildTypeSignature(const CollectedMethod& method)
{
    std::string sig;

    if (method.returnType == "void")
    {
        sig += "v";
    }
    else if (method.returnType == "int64_t" || method.returnType == "uint64_t" || method.returnType == "long" || method.returnType == "Int64")
    {
        sig += "j";
    }
    else if (method.returnType == "float" || method.returnType == "Single")
    {
        sig += "f";
    }
    else if (method.returnType == "double" || method.returnType == "Double")
    {
        sig += "d";
    }
    else
    {
        sig += "i";
    }

    if (!method.isStatic)
    {
        sig += "i";
    }

    for (const auto& param : method.params)
    {
        if (param.typeName == "int64_t" || param.typeName == "uint64_t" || param.typeName == "long" || param.typeName == "Int64")
        {
            sig += "j";
        }
        else if (param.typeName == "float" || param.typeName == "Single")
        {
            sig += "f";
        }
        else if (param.typeName == "double" || param.typeName == "Double")
        {
            sig += "d";
        }
        else
        {
            sig += "i";
        }
    }

    sig += "i";

    return sig;
}

std::string SidecarWriter::GetTypeKindStr(TypeKind kind, bool isAbstract, bool isSealed)
{
    switch (kind)
    {
    case TypeKind::Interface:
        return "interface ";
    case TypeKind::Enum:
        return "enum ";
    case TypeKind::Struct:
        return "struct ";
    case TypeKind::Delegate:
        return "delegate ";
    default:
        if (isAbstract && isSealed)
        {
            return "static class ";
        }
        if (isAbstract)
        {
            return "abstract class ";
        }
        if (isSealed)
        {
            return "sealed class ";
        }
        return "class ";
    }
}

std::string SidecarWriter::GetFieldModifiers(const CollectedField& field)
{
    std::string mod;
    if (field.isLiteral)
    {
        mod += "const ";
    }
    else
    {
        if (field.isStatic)
        {
            mod += "static ";
        }
        if (field.isReadOnly)
        {
            mod += "readonly ";
        }
    }
    return mod;
}

std::string SidecarWriter::GetMethodModifiers(const CollectedMethod& method)
{
    std::string mod;
    if (method.isStatic)
    {
        mod += "static ";
    }
    if (method.isAbstract)
    {
        mod += "abstract ";
    }
    else if (method.isSealed && method.isOverride)
    {
        mod += "sealed override ";
    }
    else if (method.isOverride)
    {
        mod += "override ";
    }
    else if (method.isVirtual)
    {
        mod += "virtual ";
    }
    if (method.isExtern)
    {
        mod += "extern ";
    }
    return mod;
}

std::string SidecarWriter::EscapeJsonString(const std::string& s)
{
    std::string result;
    result.reserve(s.size() + 16);
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                result += buf;
            }
            else
            {
                result += c;
            }
        }
    }
    return result;
}

std::string SidecarWriter::EscapeCppName(const std::string& name)
{
    static const std::set<std::string> keywords = {
        "klass", "monitor", "register", "_cs", "auto", "friend", "template",
        "new", "delete", "this", "class", "struct", "enum", "union",
        "public", "private", "protected", "virtual", "override",
        "static", "const", "volatile", "extern", "inline",
        "if", "else", "while", "for", "do", "switch", "case", "default",
        "break", "continue", "return", "goto", "try", "catch", "throw",
        "namespace", "using", "typedef", "typename", "sizeof", "alignof",
        "nullptr", "true", "false", "and", "or", "not", "xor", "bitand", "bitor", "compl",
        "and_eq", "or_eq", "not_eq", "xor_eq",
        "near", "far", "far16"
    };

    std::string result = name;

    for (char& c : result)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
        {
            c = '_';
        }
    }

    if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0])))
    {
        result = "_" + result;
    }

    if (keywords.count(result))
    {
        result = "_" + result + "_";
    }

    return result;
}

std::string SidecarWriter::ToCppType(const std::string& csharpType, bool isValueType)
{
    if (csharpType == "void")
    {
        return "void";
    }
    if (csharpType == "bool" || csharpType == "Boolean")
    {
        return "bool";
    }
    if (csharpType == "byte" || csharpType == "Byte")
    {
        return "uint8_t";
    }
    if (csharpType == "sbyte" || csharpType == "SByte")
    {
        return "int8_t";
    }
    if (csharpType == "char" || csharpType == "Char")
    {
        return "uint16_t";
    }
    if (csharpType == "short" || csharpType == "Int16")
    {
        return "int16_t";
    }
    if (csharpType == "ushort" || csharpType == "UInt16")
    {
        return "uint16_t";
    }
    if (csharpType == "int" || csharpType == "Int32")
    {
        return "int32_t";
    }
    if (csharpType == "uint" || csharpType == "UInt32")
    {
        return "uint32_t";
    }
    if (csharpType == "long" || csharpType == "Int64")
    {
        return "int64_t";
    }
    if (csharpType == "ulong" || csharpType == "UInt64")
    {
        return "uint64_t";
    }
    if (csharpType == "float" || csharpType == "Single")
    {
        return "float";
    }
    if (csharpType == "double" || csharpType == "Double")
    {
        return "double";
    }
    if (csharpType == "string" || csharpType == "String")
    {
        return "System_String_o*";
    }
    if (csharpType == "IntPtr" || csharpType == "UIntPtr")
    {
        return "intptr_t";
    }
    if (csharpType == "object" || csharpType == "Object")
    {
        return "Il2CppObject*";
    }

    if (isValueType)
    {
        return EscapeCppName(csharpType) + "_o";
    }
    return EscapeCppName(csharpType) + "_o*";
}

} // namespace er2
