#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "../metadata/metadata_header_fields.hpp"
#include "sdk_common.hpp"
#include "sdk_strings.hpp"

namespace er2
{

inline void DumpSdk6WriteJsonArrayU32(std::ostream& os, const std::vector<std::uint32_t>& a)
{
    os << "[";
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (i) os << ", ";
        os << (unsigned long long)a[i];
    }
    os << "]";
}

inline void DumpSdk6WriteJsonArrayStr(std::ostream& os, const std::vector<std::string>& a)
{
    os << "[";
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (i) os << ", ";
        os << "\"" << DumpSdk6JsonEscape(a[i]) << "\"";
    }
    os << "]";
}

inline bool DumpSdk6WriteGenericJsonFile(
    const std::string& outPath,
    const MetadataHeaderFields& header,
    const std::vector<std::uint8_t>& metaBytes,
    const std::vector<DumpSdk6GenericParamInfo>& genericParams,
    const std::vector<std::string>& typeFullName)
{
    std::ofstream ofs(outPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.good())
    {
        return false;
    }

    ofs << "{\n";

    ofs << "\t\"genericParameters\": [\n";
    for (std::size_t i = 0; i < genericParams.size(); ++i)
    {
        const std::string name = DumpSdk6JsonEscape(genericParams[i].name);
        ofs << "\t\t{ \"index\": " << (unsigned long long)i << ", \"name\": \"" << name << "\" }";
        if (i + 1 < genericParams.size())
        {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "\t],\n";

    ofs << "\t\"genericContainers\": [\n";

    const std::uint32_t gcCount = (header.genericContainersSize == 0) ? 0u : (header.genericContainersSize / 16u);
    for (std::uint32_t i = 0; i < gcCount; ++i)
    {
        const std::size_t base = static_cast<std::size_t>(header.genericContainersOffset) + static_cast<std::size_t>(i) * 16u;
        if (base + 16u > metaBytes.size())
        {
            break;
        }

        DumpSdk6GenericContainerRaw gc;
        std::memcpy(&gc, metaBytes.data() + base, sizeof(gc));

        std::vector<std::uint32_t> paramIndices;
        std::vector<std::string> paramNames;
        if (gc.typeArgc > 0 && gc.typeArgc <= 32)
        {
            paramIndices.reserve(static_cast<std::size_t>(gc.typeArgc));
            paramNames.reserve(static_cast<std::size_t>(gc.typeArgc));
            for (int k = 0; k < gc.typeArgc; ++k)
            {
                const std::int32_t pi = gc.paramStart + k;
                if (pi < 0)
                {
                    continue;
                }
                const std::uint32_t u = static_cast<std::uint32_t>(pi);
                paramIndices.push_back(u);
                if (u < genericParams.size())
                {
                    paramNames.push_back(genericParams[u].name);
                }
                else
                {
                    paramNames.push_back(std::string());
                }
            }
        }

        std::string ownerType;
        if (gc.isMethod == 0 && gc.owner >= 0)
        {
            const std::uint32_t u = static_cast<std::uint32_t>(gc.owner);
            if (u < typeFullName.size())
            {
                ownerType = typeFullName[u];
            }
        }

        ofs << "\t\t{ ";
        ofs << "\"index\": " << (unsigned long long)i;
        ofs << ", \"owner\": " << (long long)gc.owner;
        ofs << ", \"isMethod\": " << (long long)gc.isMethod;
        ofs << ", \"typeArgc\": " << (long long)gc.typeArgc;
        ofs << ", \"paramStart\": " << (long long)gc.paramStart;
        ofs << ", \"ownerTypeFullName\": \"" << DumpSdk6JsonEscape(ownerType) << "\"";
        ofs << ", \"paramIndices\": ";
        DumpSdk6WriteJsonArrayU32(ofs, paramIndices);
        ofs << ", \"paramNames\": ";
        DumpSdk6WriteJsonArrayStr(ofs, paramNames);
        ofs << " }";

        if (i + 1 < gcCount)
        {
            ofs << ",";
        }
        ofs << "\n";
    }

    ofs << "\t]\n";
    ofs << "}\n";
    return ofs.good();
}

} // namespace er2
