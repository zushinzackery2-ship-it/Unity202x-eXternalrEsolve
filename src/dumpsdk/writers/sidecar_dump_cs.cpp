#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>

#include "sidecar_writer_internal.hpp"

#include <er2/unity2/dumpsdk/offline/MetadataFlags.h>

#include <format>
#include <fstream>
#include <sstream>

namespace er2
{

namespace
{

struct AddressText
{
    uint64_t rva = 0;
    uint64_t fileOffset = 0;
    uint64_t va = 0;
    bool valid = false;
};

AddressText ResolveAddress(
    uintptr_t address,
    uint32_t rva,
    uint32_t fileOffset,
    uintptr_t moduleBase,
    bool fromOffline)
{
    AddressText result{};
    if (address == 0)
    {
        return result;
    }
    result.va = address;
    result.rva = rva != 0
        ? rva
        : (moduleBase != 0 && address >= moduleBase ? address - moduleBase : 0);
    result.fileOffset = fileOffset != 0 || fromOffline
        ? fileOffset
        : result.rva;
    result.valid = true;
    return result;
}

void WriteAttributes(
    std::stringstream& stream,
    const std::vector<std::string>& attributes,
    const char* padding)
{
    for (const std::string& attribute : attributes)
    {
        stream << padding << attribute << "\n";
    }
}

void WriteFields(std::stringstream& stream, const CollectedType& type)
{
    using namespace SidecarDetail;
    if (type.fields.empty())
    {
        return;
    }
    stream << "\n\t// Fields\n";
    for (const CollectedField& field : type.fields)
    {
        WriteAttributes(stream, field.attributes, "\t");
        stream << "\t" << BuildFieldModifiers(field) << field.typeName << " " << field.name;
        if (!field.defaultValue.empty())
        {
            stream << (field.defaultValueIsComment
                ? field.defaultValue
                : " = " + field.defaultValue);
        }
        if (field.isLiteral)
        {
            stream << ";\n";
            continue;
        }
        stream << (field.hasOffset
            ? std::format("; // 0x{:X}\n", field.offset)
            : "; // 0xFFFFFFFF\n");
    }
}

void WriteProperties(std::stringstream& stream, const CollectedType& type)
{
    if (type.properties.empty())
    {
        return;
    }
    stream << "\n\t// Properties\n";
    for (const CollectedProperty& property : type.properties)
    {
        WriteAttributes(stream, property.attributes, "\t");
        stream << "\t" << property.modifiers << property.typeName <<
            " " << property.name << " { ";
        if (property.hasGetter)
        {
            stream << "get; ";
        }
        if (property.hasSetter)
        {
            stream << "set; ";
        }
        stream << "}\n";
    }
}

void WriteEvents(std::stringstream& stream, const CollectedType& type)
{
    if (type.events.empty())
    {
        return;
    }
    stream << "\n\t// Events\n";
    for (const CollectedEvent& event : type.events)
    {
        WriteAttributes(stream, event.attributes, "\t");
        stream << "\t" << event.modifiers << "event " << event.typeName <<
            " " << event.name << ";\n";
    }
}

void WriteMethodAddress(std::stringstream& stream, const AddressText& address, uint16_t slot)
{
    if (address.valid)
    {
        stream << std::format(
            "\t// RVA: 0x{:X} Offset: 0x{:X} VA: 0x{:X}",
            address.rva,
            address.fileOffset,
            address.va);
    }
    else
    {
        stream << "\t// RVA: -1 Offset: -1";
    }
    if (slot != kNoVTableSlot)
    {
        stream << std::format(" Slot: {}", slot);
    }
}

void WriteParameters(std::stringstream& stream, const CollectedMethod& method)
{
    for (size_t i = 0; i < method.params.size(); ++i)
    {
        if (i > 0)
        {
            stream << ", ";
        }
        stream << SidecarDetail::BuildParamText(method.params[i]);
    }
}

void WriteGenericInstGroups(
    std::stringstream& stream,
    const CollectedMethod& method,
    uintptr_t moduleBase,
    bool fromOffline)
{
    if (method.genericInstGroups.empty())
    {
        return;
    }
    stream << "\t/* GenericInstMethod :\n";
    for (const CollectedGenericInstGroup& group : method.genericInstGroups)
    {
        stream << "\t|\n";
        const AddressText address = ResolveAddress(
            group.address,
            group.rva,
            group.fileOffset,
            moduleBase,
            fromOffline);
        if (address.valid)
        {
            stream << std::format(
                "\t|-RVA: 0x{:X} Offset: 0x{:X} VA: 0x{:X}\n",
                address.rva,
                address.fileOffset,
                address.va);
        }
        else
        {
            stream << "\t|-RVA: -1 Offset: -1\n";
        }
        for (const std::string& entry : group.entries)
        {
            stream << "\t|-" << entry << "\n";
        }
    }
    stream << "\t*/\n";
}

void WriteMethod(
    std::stringstream& stream,
    const CollectedMethod& method,
    uintptr_t moduleBase,
    bool fromOffline)
{
    using namespace SidecarDetail;
    stream << "\n";
    WriteAttributes(stream, method.attributes, "\t");
    WriteMethodAddress(stream, ResolveAddress(
        method.address,
        method.rva,
        method.fileOffset,
        moduleBase,
        fromOffline), method.slot);
    stream << "\n\t" << BuildMethodModifiers(method);
    if (method.returnIsByRef)
    {
        stream << "ref ";
    }
    stream << method.returnType << " " << method.name << "(";
    WriteParameters(stream, method);
    stream << (method.isAbstract ? ");\n" : ") { }\n");
    WriteGenericInstGroups(stream, method, moduleBase, fromOffline);
}

void WriteMethods(
    std::stringstream& stream,
    const CollectedType& type,
    uintptr_t moduleBase,
    bool fromOffline)
{
    if (type.methods.empty())
    {
        return;
    }
    stream << "\n\t// Methods\n";
    for (const CollectedMethod& method : type.methods)
    {
        WriteMethod(stream, method, moduleBase, fromOffline);
    }
}

void WriteType(
    std::stringstream& stream,
    const CollectedType& type,
    uintptr_t moduleBase,
    bool fromOffline)
{
    using namespace SidecarDetail;
    stream << std::format("\n// Namespace: {}\n", type.namespaceName);
    WriteAttributes(stream, type.attributes, "");
    if (type.isSerializable)
    {
        stream << "[Serializable]\n";
    }
    stream << BuildTypeDeclaration(type) << BuildBaseClause(type);
    stream << std::format(" // TypeDefIndex: {}\n{{", type.typeDefIndex);
    WriteFields(stream, type);
    WriteProperties(stream, type);
    WriteEvents(stream, type);
    WriteMethods(stream, type, moduleBase, fromOffline);
    stream << "}\n";
}

} // namespace

bool SidecarWriter::WriteDumpCs(
    const std::string& path,
    const CollectedData& data,
    std::uintptr_t moduleBase)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    std::stringstream stream;
    for (const CollectedAssembly& assembly : data.assemblies)
    {
        stream << std::format(
            "// Image {}: {} - {}\n",
            assembly.imageIndex,
            assembly.name,
            assembly.typeStartIndex);
    }
    for (const CollectedAssembly& assembly : data.assemblies)
    {
        for (const CollectedType& type : assembly.types)
        {
            WriteType(stream, type, moduleBase, data.fromOffline);
        }
    }
    output << stream.str();
    return output.good();
}

} // namespace er2
