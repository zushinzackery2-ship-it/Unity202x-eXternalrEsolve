#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>

#include "sidecar_writer_internal.hpp"

#include <format>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace er2
{

namespace
{

void WritePrelude(std::stringstream& stream)
{
    stream << "#pragma once\n\n#include <stdint.h>\n\n";
    stream << "typedef void(*Il2CppMethodPointer)();\n";
    stream << "struct MethodInfo;\nstruct Il2CppClass;\nstruct Il2CppArrayBounds;\n\n";
    stream << "struct VirtualInvokeData\n{\n";
    stream << "\tIl2CppMethodPointer methodPtr;\n\tconst MethodInfo* method;\n};\n\n";
    stream << "struct Il2CppType\n{\n\tvoid* data;\n\tunsigned int bits;\n};\n\n";
    stream << "struct Il2CppObject\n{\n\tIl2CppClass* klass;\n\tvoid* monitor;\n};\n\n";
    stream << "union Il2CppRGCTXData\n{\n\tvoid* rgctxDataDummy;\n";
    stream << "\tconst MethodInfo* method;\n\tconst Il2CppType* type;\n\tIl2CppClass* klass;\n};\n\n";
    stream << "struct System_String_o;\n\n";
}

} // namespace

bool SidecarWriter::WriteIl2CppHeader(const std::string& path, const CollectedData& data)
{
    using namespace SidecarDetail;

    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    std::stringstream stream;
    WritePrelude(stream);
    std::unordered_set<std::string> usedNames;
    auto uniqueName = [&](const CollectedType& type)
    {
        const std::string base = EscapeCppName(
            type.namespaceName.empty() ? type.name : type.namespaceName + "_" + type.name);
        if (usedNames.insert(base).second)
        {
            return base;
        }
        for (size_t suffix = 2; ; ++suffix)
        {
            const std::string candidate = base + "_" + std::to_string(suffix);
            if (usedNames.insert(candidate).second)
            {
                return candidate;
            }
        }
    };

    for (const CollectedAssembly& assembly : data.assemblies)
    {
        for (const CollectedType& type : assembly.types)
        {
            const std::string safeName = uniqueName(type);
            const bool valueLike = type.kind == TypeKind::Struct || type.kind == TypeKind::Enum;
            stream << std::format("// {}::{}\n", type.namespaceName, type.name);
            stream << std::format("struct {}_Fields\n{{\n", safeName);
            bool wroteField = false;
            for (const CollectedField& field : type.fields)
            {
                if (field.isStatic || field.isLiteral)
                {
                    continue;
                }
                stream << std::format(
                    "\t{} {}; // 0x{:X}\n",
                    ToCppType(field.typeName, valueLike),
                    EscapeCppName(field.name),
                    field.offset);
                wroteField = true;
            }
            if (!wroteField)
            {
                stream << "\tuint8_t _padding_do_not_use;\n";
            }
            stream << "};\n\n";

            if (type.isGeneric)
            {
                stream << std::format(
                    "struct {}_rgctxs\n{{\n\tIl2CppRGCTXData* rgctxData;\n}};\n\n",
                    safeName);
            }
            stream << std::format(
                "struct {}_VTable\n{{\n\tVirtualInvokeData vtable[32];\n}};\n\n",
                safeName);
            stream << std::format(
                "struct {}_c\n{{\n\tIl2CppClass* _klass;\n}};\n\n",
                safeName);
            stream << std::format("struct {}_o\n{{\n", safeName);
            if (!valueLike)
            {
                stream << std::format("\t{}_c* klass;\n\tvoid* monitor;\n", safeName);
            }
            stream << std::format("\t{}_Fields fields;\n}};\n\n", safeName);

            bool staticOpen = false;
            for (const CollectedField& field : type.fields)
            {
                if (!field.isStatic)
                {
                    continue;
                }
                if (!staticOpen)
                {
                    stream << std::format("struct {}_StaticFields\n{{\n", safeName);
                    staticOpen = true;
                }
                stream << std::format(
                    "\t{} {}; // 0x{:X}\n",
                    ToCppType(field.typeName, valueLike),
                    EscapeCppName(field.name),
                    field.offset);
            }
            if (staticOpen)
            {
                stream << "};\n\n";
            }
            stream << std::format(
                "struct {}_array\n{{\n\tIl2CppObject obj;\n\tIl2CppArrayBounds* bounds;\n"
                "\tint32_t max_length;\n\t{}_o* m_Items[65535];\n}};\n\n",
                safeName,
                safeName);
        }
    }
    output << stream.str();
    return output.good();
}

} // namespace er2
