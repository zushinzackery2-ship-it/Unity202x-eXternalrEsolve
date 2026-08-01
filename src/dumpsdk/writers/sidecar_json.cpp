#include <er2/unity2/dumpsdk/writers/sidecar_writer.hpp>

#include "sidecar_writer_internal.hpp"

#include <format>
#include <fstream>
#include <set>

namespace er2
{

namespace
{

uint64_t ToRva(uintptr_t address, uint32_t rva, uintptr_t moduleBase)
{
    if (address == 0)
    {
        return 0;
    }
    if (rva != 0)
    {
        return rva;
    }
    return moduleBase != 0 && address >= moduleBase ? address - moduleBase : 0;
}

} // namespace

bool SidecarWriter::WriteScriptJson(
    const std::string& path,
    const CollectedData& data,
    std::uintptr_t moduleBase)
{
    using namespace SidecarDetail;

    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    std::set<uint64_t> addresses;
    output << "{\n  \"ScriptMethod\": [\n";
    bool first = true;
    for (const CollectedAssembly& assembly : data.assemblies)
    {
        for (const CollectedType& type : assembly.types)
        {
            for (const CollectedMethod& method : type.methods)
            {
                const uint64_t rva = ToRva(method.address, method.rva, moduleBase);
                if (rva == 0)
                {
                    continue;
                }
                addresses.insert(rva);
                if (!first)
                {
                    output << ",\n";
                }
                first = false;
                output << "    {\n";
                output << std::format("      \"Address\": {},\n", rva);
                output << std::format(
                    "      \"Name\": \"{}\",\n",
                    EscapeJsonString(type.name + "$$" + method.name));
                output << std::format(
                    "      \"Signature\": \"{}\",\n",
                    EscapeJsonString(BuildMethodSignature(method, type.name, method.isStatic)));
                output << std::format(
                    "      \"TypeSignature\": \"{}\"\n",
                    EscapeJsonString(BuildTypeSignature(method)));
                output << "    }";
            }
        }
    }

    output << "\n  ],\n  \"ScriptString\": [\n";
    first = true;
    for (const CollectedStringLiteral& literal : data.strings)
    {
        const uint64_t rva = ToRva(literal.address, 0, moduleBase);
        if (rva == 0)
        {
            continue;
        }
        addresses.insert(rva);
        if (!first)
        {
            output << ",\n";
        }
        first = false;
        output << "    {\n";
        output << std::format("      \"Address\": {},\n", rva);
        output << std::format(
            "      \"Value\": \"{}\"\n",
            EscapeJsonString(literal.value));
        output << "    }";
    }

    output << "\n  ],\n  \"ScriptMetadata\": [\n";
    first = true;
    for (const CollectedMetadata& metadata : data.metadata)
    {
        const uint64_t rva = ToRva(metadata.address, 0, moduleBase);
        if (rva == 0)
        {
            continue;
        }
        addresses.insert(rva);
        if (!first)
        {
            output << ",\n";
        }
        first = false;
        output << "    {\n";
        output << std::format("      \"Address\": {},\n", rva);
        output << std::format("      \"Name\": \"{}\",\n", EscapeJsonString(metadata.name));
        output << std::format(
            "      \"Signature\": \"{}\"\n",
            EscapeJsonString(metadata.signature));
        output << "    }";
    }

    output << "\n  ],\n  \"ScriptMetadataMethod\": [\n";
    first = true;
    for (const CollectedMethodRef& methodRef : data.methodRefs)
    {
        const uint64_t rva = ToRva(methodRef.address, 0, moduleBase);
        if (rva == 0)
        {
            continue;
        }
        addresses.insert(rva);
        if (!first)
        {
            output << ",\n";
        }
        first = false;
        output << "    {\n";
        output << std::format("      \"Address\": {},\n", rva);
        output << std::format("      \"Name\": \"{}\",\n", EscapeJsonString(methodRef.name));
        output << std::format(
            "      \"MethodAddress\": {}\n",
            ToRva(methodRef.methodAddress, 0, moduleBase));
        output << "    }";
    }

    output << "\n  ],\n  \"Addresses\": [\n";
    first = true;
    for (const uint64_t address : addresses)
    {
        if (!first)
        {
            output << ",\n";
        }
        first = false;
        output << std::format("    {}", address);
    }
    output << "\n  ]\n}\n";
    return output.good();
}

bool SidecarWriter::WriteStringLiteralJson(
    const std::string& path,
    const CollectedData& data,
    std::uintptr_t moduleBase)
{
    (void)moduleBase;
    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    output << "[\n";
    bool first = true;
    for (const CollectedStringLiteral& literal : data.strings)
    {
        if (!first)
        {
            output << ",\n";
        }
        first = false;
        output << "  {\n";
        output << std::format(
            "    \"value\": \"{}\",\n",
            SidecarDetail::EscapeJsonString(literal.value));
        output << std::format("    \"address\": \"0x{:X}\"\n", literal.address);
        output << "  }";
    }
    output << "\n]\n";
    return output.good();
}

} // namespace er2
