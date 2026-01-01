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

#include "../metadata/metadata_header_fields.hpp"
#include "../metadata/metadata_images.hpp"
#include "../metadata/registration_helpers.hpp"
#include "../metadata/registration_types.hpp"

#include "sdk_common.hpp"
#include "sdk_generic_json.hpp"
#include "sdk_metadata_helpers.hpp"
#include "sdk_strings.hpp"
#include "sdk_type_resolver.hpp"

namespace er2
{

inline bool DumpSdk6TryRvaToFileOffset(std::uint32_t rva, const std::vector<detail_il2cpp_reg::DiskSection>& secs, std::uint64_t& out)
{
    out = 0;
    if (rva == 0)
    {
        return false;
    }

    for (const auto& s : secs)
    {
        if (s.vsize == 0)
        {
            continue;
        }
        const std::uint32_t start = s.rva;
        const std::uint32_t end = start + s.vsize;
        if (rva < start || rva >= end)
        {
            continue;
        }
        if (s.rawPtr == 0)
        {
            return false;
        }
        out = static_cast<std::uint64_t>(s.rawPtr) + static_cast<std::uint64_t>(rva - start);
        return true;
    }

    return false;
}

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

        ofs << "// Namespace: " << ns << "\n";
        ofs << access << " " << kind << " " << displayName << typeSuffix << inheritClause << " // TypeDefIndex: " << (unsigned long long)i << "\n";
        ofs << "{\n";
        ofs << "\t// Token: 0x";
        ofs << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned long long)td->token << std::dec << std::setfill(' ');
        ofs << "\n";
        if (!parentDecl.empty())
        {
            ofs << "\t// Parent: " << parentDecl << "\n";
        }
        if (!ifaceNames.empty())
        {
            std::vector<std::string> uniq;
            for (const auto& s : ifaceNames)
            {
                if (std::find(uniq.begin(), uniq.end(), s) == uniq.end())
                {
                    uniq.push_back(s);
                }
            }
            ofs << "\t// Interfaces: ";
            for (std::size_t k = 0; k < uniq.size(); ++k)
            {
                if (k) ofs << ", ";
                ofs << uniq[k];
            }
            ofs << "\n";
        }

        ofs << "\t// Fields\n";
        std::uintptr_t fieldOffArr = 0;
        if (fieldOffsetsPtr && td->fieldCount > 0)
        {
            (void)ReadPtr(mem, fieldOffsetsPtr + static_cast<std::uintptr_t>(i) * 8u, fieldOffArr);
        }

        for (std::uint32_t fi = 0; fi < td->fieldCount; ++fi)
        {
            const std::uint32_t fieldIndex = static_cast<std::uint32_t>(td->fieldStart) + fi;
            const std::size_t fbase = static_cast<std::size_t>(header.fieldsOffset) + static_cast<std::size_t>(fieldIndex) * sizeof(DumpSdk6FieldDefRaw);
            if (fbase + sizeof(DumpSdk6FieldDefRaw) > metaBytes.size())
            {
                break;
            }
            const DumpSdk6FieldDefRaw* fd = reinterpret_cast<const DumpSdk6FieldDefRaw*>(metaBytes.data() + fbase);

            std::string fname;
            (void)ReadCStringFromMetadataBytes(metaBytes, header, fd->nameIndex, fname);
            if (fname.empty())
            {
                fname = "field" + std::to_string(fi);
            }

            std::string ftype = DumpSdk6ToCsType(resolver.DescribeFromTypeIndex(fd->typeIndex));

            ofs << "\tpublic " << ftype << " " << fname << ";";

            if (fieldOffArr)
            {
                std::int32_t offVal = 0;
                if (ReadValue(mem, fieldOffArr + static_cast<std::uintptr_t>(fi) * 4u, offVal))
                {
                    std::uint32_t offU = static_cast<std::uint32_t>(offVal);
                    if ((kind == "struct" || kind == "enum") && offU >= 0x10u)
                    {
                        offU -= 0x10u;
                    }
                    ofs << " // 0x" << std::hex << std::uppercase << (unsigned long long)offU << std::dec;
                }
            }

            ofs << " Token: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned long long)fd->token << std::dec << std::setfill(' ') << "\n";
        }

        ofs << "\n";
        ofs << "\t// Methods\n";

        for (std::uint32_t mi = 0; mi < td->methodCount; ++mi)
        {
            const std::uint32_t methodIndex = static_cast<std::uint32_t>(td->methodStart) + mi;
            DumpSdk6MethodDefFull md;
            if (!ReadMethodDefFullFromBytes(metaBytes, header, methodLayout, methodIndex, md))
            {
                break;
            }

            std::string mname;
            (void)ReadCStringFromMetadataBytes(metaBytes, header, md.nameIndex, mname);
            if (mname.empty())
            {
                mname = "method" + std::to_string(mi);
            }

            const bool isCtor = (mname == ".ctor" || mname == ".cctor");
            const std::string mods = MethodModifiersFromFlags(md.flags, isCtor);

            std::string returnType = "void";
            if (!isCtor)
            {
                returnType = DumpSdk6ToCsType(resolver.DescribeFromTypeIndex(md.returnType));
                if (returnType.empty())
                {
                    returnType = "void";
                }
            }

            std::vector<std::string> params;
            params.reserve(md.parameterCount);
            for (std::uint32_t pi = 0; pi < md.parameterCount; ++pi)
            {
                const std::uint32_t pIndex = static_cast<std::uint32_t>(md.parameterStart) + pi;
                const std::size_t pbase = static_cast<std::size_t>(header.parametersOffset) + static_cast<std::size_t>(pIndex) * sizeof(DumpSdk6ParamDefRaw);
                if (pbase + sizeof(DumpSdk6ParamDefRaw) > metaBytes.size())
                {
                    break;
                }
                const DumpSdk6ParamDefRaw* pd = reinterpret_cast<const DumpSdk6ParamDefRaw*>(metaBytes.data() + pbase);

                std::string pname;
                (void)ReadCStringFromMetadataBytes(metaBytes, header, pd->nameIndex, pname);
                if (pname.empty())
                {
                    pname = "arg" + std::to_string(pi);
                }
                std::string ptype = DumpSdk6ToCsType(resolver.DescribeFromTypeIndex(pd->typeIndex));
                if (ptype.empty())
                {
                    ptype = "object";
                }
                params.push_back(ptype + " " + pname);
            }

            std::string paramsStr;
            for (std::size_t k = 0; k < params.size(); ++k)
            {
                if (k) paramsStr += ", ";
                paramsStr += params[k];
            }

            std::uintptr_t va = 0;
            std::uint64_t rva = 0;
            std::uint64_t fileOff = 0;

            const std::string imgName = (i < typeToImage.size()) ? typeToImage[i] : std::string();
            const std::string key = DumpSdk6NormalizeAssemblyKey(imgName);

            const std::uint32_t rowId = md.token & 0x00FFFFFFu;
            if (rowId != 0 && !key.empty())
            {
                auto it = modulePointers.find(key);
                if (it != modulePointers.end())
                {
                    const std::uint64_t idx = static_cast<std::uint64_t>(rowId - 1u);
                    if (idx < it->second.size())
                    {
                        va = static_cast<std::uintptr_t>(it->second[static_cast<std::size_t>(idx)]);
                    }
                }
            }

            if (va != 0 && va >= hint.moduleBase)
            {
                rva = static_cast<std::uint64_t>(va - hint.moduleBase);
                if (!diskSecs.empty())
                {
                    (void)DumpSdk6TryRvaToFileOffset(static_cast<std::uint32_t>(rva), diskSecs, fileOff);
                }
            }

            ofs << "\n";
            ofs << "\t// RVA: 0x" << std::hex << std::uppercase << (unsigned long long)rva << std::dec;
            ofs << " Offset: 0x" << std::hex << std::uppercase << (unsigned long long)fileOff << std::dec;
            ofs << " VA: 0x" << std::hex << std::uppercase << (unsigned long long)va << std::dec << "\n";
            ofs << "\t// Token: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned long long)md.token << std::dec << std::setfill(' ') << "\n";
            ofs << "\t" << mods << " " << returnType << " " << mname << "(" << paramsStr << ") { }\n";
        }

        ofs << "}\n\n";
    }

    return ofs.good();
}

} // namespace er2
