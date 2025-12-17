#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "Struct.hpp"

namespace UnityExternal
{

namespace detail_metadata
{

inline std::string WideToUtf8(const std::wstring& s)
{
    if (s.empty()) return std::string();
    int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bytesNeeded <= 0) return std::string();
    std::string out;
    out.resize((std::size_t)bytesNeeded - 1);
    if (!out.empty())
    {
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), bytesNeeded, nullptr, nullptr);
    }
    return out;
}

inline std::string JsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

inline void WriteJsonStringOrNull(std::ostream& os, const std::string& s)
{
    if (s.empty())
    {
        os << "null";
        return;
    }
    os << '"' << JsonEscape(s) << '"';
}

inline std::string HexU64(std::uint64_t v, int width)
{
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << v;
    return oss.str();
}

inline std::string HexU64NoPad(std::uint64_t v)
{
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << v;
    return oss.str();
}

}

inline std::string BuildMetadataHintJson(const MetadataHint& hint)
{
    std::ostringstream ofs;

    std::string procName = detail_metadata::WideToUtf8(hint.processName);
    std::string modName = detail_metadata::WideToUtf8(hint.moduleName);
    std::string modPath = detail_metadata::WideToUtf8(hint.modulePath);

    ofs << "{\n";
    ofs << "  \"schema\": " << hint.schema << ",\n";

    ofs << "  \"process\": {\n";
    ofs << "    \"name\": ";
    detail_metadata::WriteJsonStringOrNull(ofs, procName);
    ofs << ",\n";
    ofs << "    \"pid\": " << (std::uint64_t)hint.pid << "\n";
    ofs << "  },\n";

    ofs << "  \"module\": {\n";
    ofs << "    \"name\": ";
    detail_metadata::WriteJsonStringOrNull(ofs, modName);
    ofs << ",\n";
    ofs << "    \"base\": \"" << detail_metadata::HexU64((std::uint64_t)hint.moduleBase, 16) << "\",\n";
    ofs << "    \"size\": \"0x" << std::uppercase << std::hex << hint.moduleSize << std::dec << "\",\n";
    ofs << "    \"path\": ";
    detail_metadata::WriteJsonStringOrNull(ofs, modPath);
    ofs << ",\n";
    ofs << "    \"pe_image_base\": ";
    if (hint.peImageBase)
    {
        ofs << "\"" << detail_metadata::HexU64NoPad((std::uint64_t)hint.peImageBase) << "\"\n";
    }
    else
    {
        ofs << "null\n";
    }
    ofs << "  },\n";

    ofs << "  \"metadata\": {\n";
    ofs << "    \"s_global_metadata_addr\": \"" << detail_metadata::HexU64((std::uint64_t)hint.sGlobalMetadataAddr, 16) << "\",\n";
    ofs << "    \"meta_base\": \"" << detail_metadata::HexU64((std::uint64_t)hint.metaBase, 16) << "\",\n";
    ofs << "    \"total_size\": " << (std::uint64_t)hint.totalSize << ",\n";
    ofs << "    \"magic\": \"0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << hint.magic << std::dec << "\",\n";
    ofs << "    \"version\": " << (std::uint64_t)hint.version << ",\n";
    ofs << "    \"images_count\": " << (hint.imagesCount ? std::to_string((std::uint64_t)hint.imagesCount) : std::string("null")) << ",\n";
    ofs << "    \"assemblies_count\": " << (hint.assembliesCount ? std::to_string((std::uint64_t)hint.assembliesCount) : std::string("null")) << "\n";
    ofs << "  },\n";

    ofs << "  \"il2cpp\": {\n";
    ofs << "    \"code_registration\": ";
    if (hint.codeRegistration)
    {
        ofs << "\"" << detail_metadata::HexU64((std::uint64_t)hint.codeRegistration, 16) << "\"";
    }
    else
    {
        ofs << "null";
    }
    ofs << ",\n";

    ofs << "    \"metadata_registration\": ";
    if (hint.metadataRegistration)
    {
        ofs << "\"" << detail_metadata::HexU64((std::uint64_t)hint.metadataRegistration, 16) << "\"";
    }
    else
    {
        ofs << "null";
    }
    ofs << ",\n";

    ofs << "    \"code_registration_rva\": ";
    if (hint.codeRegistrationRva)
    {
        ofs << "\"" << detail_metadata::HexU64NoPad((std::uint64_t)hint.codeRegistrationRva) << "\"";
    }
    else
    {
        ofs << "null";
    }
    ofs << ",\n";

    ofs << "    \"metadata_registration_rva\": ";
    if (hint.metadataRegistrationRva)
    {
        ofs << "\"" << detail_metadata::HexU64NoPad((std::uint64_t)hint.metadataRegistrationRva) << "\"";
    }
    else
    {
        ofs << "null";
    }
    ofs << "\n";
    ofs << "  }\n";
    ofs << "}\n";

    return ofs.str();
}

}

#endif
