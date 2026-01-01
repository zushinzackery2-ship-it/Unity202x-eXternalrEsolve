#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../metadata/metadata_header_fields.hpp"
#include "sdk_common.hpp"

namespace er2
{

inline bool ReadCStringFromMetadataBytes(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    std::int32_t index,
    std::string& out)
{
    out.clear();
    if (index < 0 || h.stringOffset == 0 || h.stringSize == 0)
    {
        return false;
    }

    const std::uint32_t u = static_cast<std::uint32_t>(index);
    if (u >= h.stringSize)
    {
        return false;
    }

    const std::size_t start = static_cast<std::size_t>(h.stringOffset) + static_cast<std::size_t>(u);
    const std::size_t maxEnd = static_cast<std::size_t>(h.stringOffset) + static_cast<std::size_t>(h.stringSize);
    if (start >= meta.size())
    {
        return false;
    }

    std::size_t end = start;
    while (end < meta.size() && end < maxEnd)
    {
        if (meta[end] == 0)
        {
            break;
        }
        ++end;
    }

    out.assign(reinterpret_cast<const char*>(meta.data() + start), reinterpret_cast<const char*>(meta.data() + end));
    return !out.empty();
}

inline std::string TypeAccessFromFlags(std::uint32_t flags)
{
    const std::uint32_t vis = flags & 0x7u;
    if (vis == 0x1u) return "public";
    if (vis == 0x0u) return "internal";
    if (vis == 0x2u) return "public";
    return "internal";
}

inline std::string MethodAccessFromFlags(std::uint32_t flags)
{
    const std::uint32_t access = flags & 0x7u;
    if (access == 0x6u) return "public";
    if (access == 0x1u) return "private";
    if (access == 0x4u) return "protected";
    if (access == 0x3u) return "internal";
    if (access == 0x2u) return "private protected";
    if (access == 0x5u) return "protected internal";
    return "private";
}

inline std::string MethodModifiersFromFlags(std::uint32_t flags, bool isCtor)
{
    std::string out = MethodAccessFromFlags(flags);
    if ((flags & 0x0010u) != 0u)
    {
        out += " static";
    }
    if (!isCtor && (flags & 0x0400u) != 0u)
    {
        out += " abstract";
    }
    else if (!isCtor && (flags & 0x0040u) != 0u)
    {
        out += " virtual";
    }
    return out;
}

inline bool IsLikelyMethodToken(std::uint32_t token)
{
    return (token & 0xFF000000u) == 0x06000000u && (token & 0x00FFFFFFu) != 0u;
}

inline bool DetectMethodDefLayoutFullFromHeader(const std::vector<std::uint8_t>& meta, const MetadataHeaderFields& h, DumpSdk6MethodDefLayout& out)
{
    out = DumpSdk6MethodDefLayout{};
    if (h.methodsOffset == 0 || h.methodsSize == 0)
    {
        return false;
    }

    struct Candidate
    {
        std::size_t stride;
        std::size_t tokenOff;
        std::size_t returnTypeOff;
        std::size_t paramStartOff;
        std::size_t genericContainerOff;
        std::size_t flagsOff;
        std::size_t paramCountOff;
    };

    const Candidate candidates[] = {
        {0x20u, 0x14u, 0x08u, 0x0Cu, 0x10u, 0x18u, 0x1Eu},
        {0x24u, 0x18u, 0x08u, 0x10u, 0x14u, 0x1Cu, 0x22u},
    };

    int bestScore = -1;

    for (const auto& c : candidates)
    {
        if ((h.methodsSize % static_cast<std::uint32_t>(c.stride)) != 0u)
        {
            continue;
        }

        const std::uint32_t cnt = h.methodsSize / static_cast<std::uint32_t>(c.stride);
        if (cnt == 0 || cnt > 800000u)
        {
            continue;
        }

        const std::uint32_t sample = (cnt > 512u) ? 512u : cnt;
        int ok = 0;
        for (std::uint32_t i = 0; i < sample; ++i)
        {
            const std::size_t base = static_cast<std::size_t>(h.methodsOffset) + static_cast<std::size_t>(i) * c.stride;
            const std::uint32_t tok = ReadU32LEBytes(meta, base + c.tokenOff);
            if (IsLikelyMethodToken(tok))
            {
                ++ok;
            }
        }

        if (ok > bestScore)
        {
            bestScore = ok;
            out.stride = c.stride;
            out.tokenOffset = c.tokenOff;
            out.returnTypeOffset = c.returnTypeOff;
            out.parameterStartOffset = c.paramStartOff;
            out.genericContainerOffset = c.genericContainerOff;
            out.flagsOffset = c.flagsOff;
            out.parameterCountOffset = c.paramCountOff;
        }
    }

    return out.stride != 0;
}

inline bool ReadMethodDefFullFromBytes(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    const DumpSdk6MethodDefLayout& layout,
    std::uint32_t methodDefIndex,
    DumpSdk6MethodDefFull& out)
{
    out = DumpSdk6MethodDefFull{};
    if (layout.stride == 0 || h.methodsOffset == 0)
    {
        return false;
    }

    const std::size_t base = static_cast<std::size_t>(h.methodsOffset) + static_cast<std::size_t>(methodDefIndex) * layout.stride;
    if (base + layout.stride > meta.size())
    {
        return false;
    }

    out.nameIndex = ReadI32LEBytes(meta, base + 0x00u);
    out.declaringType = ReadI32LEBytes(meta, base + 0x04u);
    out.returnType = ReadI32LEBytes(meta, base + layout.returnTypeOffset);
    out.parameterStart = ReadI32LEBytes(meta, base + layout.parameterStartOffset);
    out.genericContainerIndex = ReadI32LEBytes(meta, base + layout.genericContainerOffset);
    out.token = ReadU32LEBytes(meta, base + layout.tokenOffset);

    out.flags = ReadU16LEBytes(meta, base + layout.flagsOffset);
    out.iflags = ReadU16LEBytes(meta, base + layout.flagsOffset + 2u);
    out.slot = ReadU16LEBytes(meta, base + layout.flagsOffset + 4u);
    out.parameterCount = ReadU16LEBytes(meta, base + layout.parameterCountOffset);
    return true;
}

inline std::vector<DumpSdk6GenericParamInfo> BuildGenericParamInfoFromBytes(const std::vector<std::uint8_t>& meta, const MetadataHeaderFields& h)
{
    std::vector<DumpSdk6GenericParamInfo> out;
    if (h.genericParametersOffset == 0 || h.genericParametersSize == 0)
    {
        return out;
    }

    const std::uint32_t count = h.genericParametersSize / 16u;
    out.resize(count);

    for (std::uint32_t i = 0; i < count; ++i)
    {
        const std::size_t base = static_cast<std::size_t>(h.genericParametersOffset) + static_cast<std::size_t>(i) * 16u;
        if (base + 16u > meta.size())
        {
            break;
        }

        const std::int32_t nameIndex = ReadI32LEBytes(meta, base + 0x04u);
        std::string name;
        (void)ReadCStringFromMetadataBytes(meta, h, nameIndex, name);
        out[i].name = name;
    }

    return out;
}

inline bool BuildTypeFullNameAndByvalMapFromBytes(
    const std::vector<std::uint8_t>& meta,
    const MetadataHeaderFields& h,
    std::vector<std::string>& outTypeFullName,
    std::unordered_map<std::uint32_t, std::string>& outByvalToFullName)
{
    outTypeFullName.clear();
    outByvalToFullName.clear();

    if (h.typeDefinitionsOffset == 0 || h.typeDefinitionsSize == 0)
    {
        return false;
    }

    const std::uint32_t typeDefCount = h.typeDefinitionsSize / static_cast<std::uint32_t>(sizeof(DumpSdk6TypeDefRaw));
    outTypeFullName.resize(typeDefCount);

    for (std::uint32_t i = 0; i < typeDefCount; ++i)
    {
        const std::size_t base = static_cast<std::size_t>(h.typeDefinitionsOffset) + static_cast<std::size_t>(i) * sizeof(DumpSdk6TypeDefRaw);
        if (base + sizeof(DumpSdk6TypeDefRaw) > meta.size())
        {
            break;
        }

        const DumpSdk6TypeDefRaw* td = reinterpret_cast<const DumpSdk6TypeDefRaw*>(meta.data() + base);

        std::string n;
        std::string ns;
        (void)ReadCStringFromMetadataBytes(meta, h, td->nameIndex, n);
        (void)ReadCStringFromMetadataBytes(meta, h, td->namespaceIndex, ns);

        std::string full = ns.empty() ? n : (ns + "." + n);
        outTypeFullName[i] = full;

        if (td->byvalTypeIndex >= 0)
        {
            outByvalToFullName[static_cast<std::uint32_t>(td->byvalTypeIndex)] = full;
        }
    }

    return true;
}

inline std::string FormatGenericSuffixForTypeFromBytes(const std::vector<std::uint8_t>& meta, const MetadataHeaderFields& h, std::int32_t genericContainerIndex)
{
    if (genericContainerIndex < 0 || h.genericContainersOffset == 0 || h.genericContainersSize == 0)
    {
        return std::string();
    }

    const std::uint32_t count = h.genericContainersSize / 16u;
    const std::uint32_t idx = static_cast<std::uint32_t>(genericContainerIndex);
    if (idx >= count)
    {
        return std::string();
    }

    const std::size_t base = static_cast<std::size_t>(h.genericContainersOffset) + static_cast<std::size_t>(idx) * 16u;
    if (base + 16u > meta.size())
    {
        return std::string();
    }

    DumpSdk6GenericContainerRaw gc;
    std::memcpy(&gc, meta.data() + base, sizeof(gc));
    if (gc.isMethod != 0 || gc.typeArgc <= 0 || gc.typeArgc > 32)
    {
        return std::string();
    }

    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(gc.typeArgc));

    for (int i = 0; i < gc.typeArgc; ++i)
    {
        const std::int32_t gpIndex = gc.paramStart + i;
        if (gpIndex < 0 || h.genericParametersOffset == 0)
        {
            break;
        }

        const std::size_t gpBase = static_cast<std::size_t>(h.genericParametersOffset) + static_cast<std::size_t>(gpIndex) * 16u;
        if (gpBase + 16u > meta.size())
        {
            break;
        }

        const std::int32_t nameIndex = ReadI32LEBytes(meta, gpBase + 0x04u);
        std::string n;
        (void)ReadCStringFromMetadataBytes(meta, h, nameIndex, n);
        if (n.empty())
        {
            n = "T" + std::to_string(i);
        }
        args.push_back(n);
    }

    if (args.empty())
    {
        return std::string();
    }

    std::string suffix;
    suffix.reserve(4 + args.size() * 8);
    suffix.push_back('<');
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        if (i) suffix += ", ";
        suffix += args[i];
    }
    suffix.push_back('>');
    return suffix;
}

} // namespace er2
