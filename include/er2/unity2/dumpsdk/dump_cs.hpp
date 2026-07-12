#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../mem/memory_accessor.hpp"
#include "../../mem/memory_read.hpp"
#include "../metadata/header/metadata_header_fields.hpp"
#include "../metadata/hint/hint_struct.hpp"
#include "../metadata/metadata_images.hpp"
#include "../metadata/registration/registration_helpers.hpp"
#include "../metadata/registration/registration_types.hpp"

#include "sdk_common.hpp"
#include "sdk_generic_json.hpp"
#include "sdk_metadata_helpers.hpp"
#include "sdk_strings.hpp"
#include "sdk_type_resolver.hpp"

#include "dump_types.hpp"
#include "dump_writer.hpp"

namespace er2
{

inline bool DumpSdk6WriteDumpCsFile(
    const std::string& outPath,
    const IMemoryAccessor& mem,
    const MetadataHint& hint,
    const MetadataHeaderFields& header,
    const std::vector<std::uint8_t>& metaBytes,
    const std::vector<MetadataImageInfo>& images,
    const std::vector<std::string>& typeToImage,
    const std::vector<std::string>& typeFullName,
    const std::unordered_map<std::uint32_t, std::string>& typeMap,
    const std::vector<DumpSdk6GenericParamInfo>& genericParams,
    std::uintptr_t typesPtr,
    std::uint32_t typesCount,
    std::uintptr_t fieldOffsetsPtr)
{
    (void)typeFullName;

    std::ofstream ofs(outPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.good())
    {
        return false;
    }

    std::vector<detail_il2cpp_reg::DiskSection> diskSecs;
    if (!hint.modulePath.empty())
    {
        (void)detail_il2cpp_reg::GetDiskPeSections(std::filesystem::path(hint.modulePath), diskSecs);
    }

    std::unordered_map<std::string, std::vector<std::uint64_t>> modulePointers;
    modulePointers.reserve(hint.codeGenModuleList.size());

    for (const auto& m : hint.codeGenModuleList)
    {
        const std::string key = DumpSdk6NormalizeAssemblyKey(m.name);
        if (key.empty())
        {
            continue;
        }
        if (m.methodPointers == 0 || m.methodPointerCount == 0 || m.methodPointerCount > 4000000u)
        {
            continue;
        }

        std::vector<std::uint64_t> buf;
        buf.resize(static_cast<std::size_t>(m.methodPointerCount));
        if (!mem.Read(m.methodPointers, buf.data(), buf.size() * sizeof(std::uint64_t)))
        {
            continue;
        }
        modulePointers[key] = std::move(buf);
    }

    DumpSdk6MethodDefLayout methodLayout;
    if (!DetectMethodDefLayoutFullFromHeader(metaBytes, header, methodLayout))
    {
        return false;
    }

    DumpSdk6TypeResolver resolver(mem, hint.metaBase, header, metaBytes, typesPtr, typesCount, typeMap, genericParams);

    for (const auto& img : images)
    {
        ofs << "// Image " << (unsigned long long)img.index << ": " << img.name << " - " << (unsigned long long)img.typeStart << "\n";
    }
    if (!images.empty())
    {
        ofs << "\n";
    }

    const std::uint32_t typeDefCount = header.typeDefinitionsSize / static_cast<std::uint32_t>(sizeof(DumpSdk6TypeDefRaw));

    for (std::uint32_t i = 0; i < typeDefCount; ++i)
    {
        const std::size_t base = static_cast<std::size_t>(header.typeDefinitionsOffset) + static_cast<std::size_t>(i) * sizeof(DumpSdk6TypeDefRaw);
        if (base + sizeof(DumpSdk6TypeDefRaw) > metaBytes.size())
        {
            break;
        }

        const DumpSdk6TypeDefRaw* td = reinterpret_cast<const DumpSdk6TypeDefRaw*>(metaBytes.data() + base);

        std::string name;
        std::string ns;
        (void)ReadCStringFromMetadataBytes(metaBytes, header, td->nameIndex, name);
        (void)ReadCStringFromMetadataBytes(metaBytes, header, td->namespaceIndex, ns);

        const std::string parentFull = (td->parentIndex >= 0) ? resolver.DescribeFromTypeIndex(td->parentIndex) : std::string();
        const std::string parentDecl = parentFull.empty() ? std::string() : DumpSdk6StripNamespacesInType(DumpSdk6ToCsType(parentFull));

        std::string kind = "class";
        if ((td->flags & 0x20u) != 0u)
        {
            kind = "interface";
        }
        else if (parentFull == "System.Enum")
        {
            kind = "enum";
        }
        else if (parentFull == "System.ValueType")
        {
            kind = "struct";
        }

        const std::string access = TypeAccessFromFlags(td->flags);

        std::string fullName;
        if (td->declaringTypeIndex >= 0)
        {
            const std::string declaringFull = resolver.DescribeFromTypeIndex(td->declaringTypeIndex);
            if (!declaringFull.empty() && !name.empty())
            {
                fullName = declaringFull + "." + name;
            }
        }
        if (fullName.empty())
        {
            fullName = ns.empty() ? name : (ns + "." + name);
        }

        std::string displayName = fullName;
        if (!ns.empty() && fullName.rfind(ns + ".", 0) == 0)
        {
            displayName = fullName.substr(ns.size() + 1);
        }

        const std::string typeSuffix = FormatGenericSuffixForTypeFromBytes(metaBytes, header, td->genericContainerIndex);

        std::vector<std::string> ifaceNames;
        if (header.interfacesOffset && td->interfacesCount > 0)
        {
            const std::uint32_t maxIf = (td->interfacesCount > 512) ? 512 : td->interfacesCount;
            const std::size_t ifBase = static_cast<std::size_t>(header.interfacesOffset) + static_cast<std::size_t>(td->interfacesStart) * 4u;
            for (std::uint32_t j = 0; j < maxIf; ++j)
            {
                const std::size_t off2 = ifBase + static_cast<std::size_t>(j) * 4u;
                if (off2 + 4u > metaBytes.size())
                {
                    break;
                }
                const std::int32_t ifaceTypeIdx = ReadI32LEBytes(metaBytes, off2);
                std::string ifaceName = DumpSdk6ToCsType(resolver.DescribeFromTypeIndex(ifaceTypeIdx));
                ifaceName = DumpSdk6StripNamespacesInType(ifaceName);
                if (!ifaceName.empty())
                {
                    ifaceNames.push_back(ifaceName);
                }
            }
        }

        std::string inheritClause;
        if (kind == "class")
        {
            if (!parentDecl.empty() && parentDecl != displayName)
            {
                inheritClause = " : " + parentDecl;
            }
        }
        else if ((kind == "struct" || kind == "interface") && !ifaceNames.empty())
        {
            std::vector<std::string> uniq;
            for (const auto& s : ifaceNames)
            {
                if (std::find(uniq.begin(), uniq.end(), s) == uniq.end())
                {
                    uniq.push_back(s);
                }
            }
            if (!uniq.empty())
            {
                inheritClause = " : ";
                for (std::size_t k = 0; k < uniq.size(); ++k)
                {
                    if (k) inheritClause += ", ";
                    inheritClause += uniq[k];
                }
            }
        }

        DumpSdk6WriteTypeHeader(ofs, ns, access, kind, displayName, typeSuffix, inheritClause, parentDecl, ifaceNames, td->token, i);
        DumpSdk6WriteFields(ofs, mem, header, metaBytes, resolver, td, fieldOffsetsPtr, i, kind);
        DumpSdk6WriteProperties(ofs, header, metaBytes, resolver, td, methodLayout);
        DumpSdk6WriteMethods(ofs, mem, hint, header, metaBytes, resolver, td, methodLayout, modulePointers, typeToImage, diskSecs, i);

        ofs << "}\n\n";
    }

    return ofs.good();
}

} // namespace er2
