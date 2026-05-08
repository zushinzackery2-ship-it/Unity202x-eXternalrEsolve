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
#include "../metadata/metadata_images.hpp"
#include "../metadata/registration/registration_helpers.hpp"
#include "../metadata/registration/registration_types.hpp"

#include "sdk_common.hpp"
#include "sdk_generic_json.hpp"
#include "sdk_metadata_helpers.hpp"
#include "sdk_strings.hpp"
#include "sdk_type_resolver.hpp"

#include "dump_types.hpp"

namespace er2
{

inline void DumpSdk6WriteTypeHeader(
    std::ofstream& ofs,
    const std::string& ns,
    const std::string& access,
    const std::string& kind,
    const std::string& displayName,
    const std::string& typeSuffix,
    const std::string& inheritClause,
    const std::string& parentDecl,
    const std::vector<std::string>& ifaceNames,
    std::uint32_t token,
    std::uint32_t typeIndex)
{
    ofs << "// Namespace: " << ns << "\n";
    ofs << access << " " << kind << " " << displayName << typeSuffix << inheritClause << " // TypeDefIndex: " << (unsigned long long)typeIndex << "\n";
    ofs << "{\n";
    ofs << "\t// Token: 0x";
    ofs << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << (unsigned long long)token << std::dec << std::setfill(' ');
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
}

inline void DumpSdk6WriteFields(
    std::ofstream& ofs,
    const IMemoryAccessor& mem,
    const MetadataHeaderFields& header,
    const std::vector<std::uint8_t>& metaBytes,
    const DumpSdk6TypeResolver& resolver,
    const DumpSdk6TypeDefRaw* td,
    std::uintptr_t fieldOffsetsPtr,
    std::uint32_t typeIndex,
    const std::string& kind)
{
    ofs << "\t// Fields\n";

    std::uintptr_t fieldOffArr = 0;
    if (fieldOffsetsPtr && td->fieldCount > 0)
    {
        (void)ReadPtr(mem, fieldOffsetsPtr + static_cast<std::uintptr_t>(typeIndex) * 8u, fieldOffArr);
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
}

inline void DumpSdk6WriteProperties(
    std::ofstream& ofs,
    const MetadataHeaderFields& header,
    const std::vector<std::uint8_t>& metaBytes,
    const DumpSdk6TypeResolver& resolver,
    const DumpSdk6TypeDefRaw* td,
    const DumpSdk6MethodDefLayout& methodLayout)
{
    if (td->methodCount == 0)
    {
        return;
    }

    std::vector<std::tuple<std::string, std::string, bool, bool>> props;
    std::unordered_map<std::string, std::size_t> propIndex;

    for (std::uint32_t mi = 0; mi < td->methodCount; ++mi)
    {
        const std::uint32_t methodIndex = static_cast<std::uint32_t>(td->methodStart) + mi;
        DumpSdk6MethodDefFull md;

        if (!ReadMethodDefFullFromBytes(metaBytes, header, methodLayout, methodIndex, md))
        {
            continue;
        }

        std::string mname;
        (void)ReadCStringFromMetadataBytes(metaBytes, header, md.nameIndex, mname);

        bool isGetter = (mname.size() > 4 && mname.substr(0, 4) == "get_");
        bool isSetter = (mname.size() > 4 && mname.substr(0, 4) == "set_");

        if (!isGetter && !isSetter)
        {
            continue;
        }

        std::string propName = mname.substr(4);
        std::string propType;

        if (isGetter)
        {
            propType = DumpSdk6ToCsType(resolver.DescribeFromTypeIndex(md.returnType));
        }
        else if (isSetter && md.parameterCount > 0)
        {
            const std::size_t paramBase = static_cast<std::size_t>(header.parametersOffset) + static_cast<std::size_t>(md.parameterStart) * sizeof(DumpSdk6ParamDefRaw);
            if (paramBase + sizeof(DumpSdk6ParamDefRaw) <= metaBytes.size())
            {
                const DumpSdk6ParamDefRaw* paramDef = reinterpret_cast<const DumpSdk6ParamDefRaw*>(metaBytes.data() + paramBase);
                propType = DumpSdk6ToCsType(resolver.DescribeFromTypeIndex(paramDef->typeIndex));
            }
        }

        auto it = propIndex.find(propName);
        if (it == propIndex.end())
        {
            propIndex[propName] = props.size();
            props.push_back(std::make_tuple(propName, propType.empty() ? "object" : propType, isGetter, isSetter));
        }
        else
        {
            auto& p = props[it->second];
            if (isGetter)
            {
                std::get<2>(p) = true;
                if (!propType.empty() && std::get<1>(p) == "object")
                {
                    std::get<1>(p) = propType;
                }
            }
            if (isSetter)
            {
                std::get<3>(p) = true;
                if (!propType.empty() && std::get<1>(p) == "object")
                {
                    std::get<1>(p) = propType;
                }
            }
        }
    }

    if (!props.empty())
    {
        ofs << "\n";
        ofs << "\t// Properties\n";
        for (const auto& p : props)
        {
            ofs << "\tpublic " << std::get<1>(p) << " " << std::get<0>(p) << " {";
            if (std::get<2>(p)) ofs << " get;";
            if (std::get<3>(p)) ofs << " set;";
            ofs << " }\n";
        }
    }
}

inline void DumpSdk6WriteMethods(
    std::ofstream& ofs,
    const IMemoryAccessor& mem,
    const MetadataHint& hint,
    const MetadataHeaderFields& header,
    const std::vector<std::uint8_t>& metaBytes,
    const DumpSdk6TypeResolver& resolver,
    const DumpSdk6TypeDefRaw* td,
    const DumpSdk6MethodDefLayout& methodLayout,
    const std::unordered_map<std::string, std::vector<std::uint64_t>>& modulePointers,
    const std::vector<std::string>& typeToImage,
    const std::vector<detail_il2cpp_reg::DiskSection>& diskSecs,
    std::uint32_t typeIndex)
{
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

        const std::string imgName = (typeIndex < typeToImage.size()) ? typeToImage[typeIndex] : std::string();
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
}

} // namespace er2
