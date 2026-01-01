#pragma once

#include <Windows.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "hint_struct.hpp"
#include "../util/json.hpp"

namespace er2
{

namespace detail_metadata_hint
{

inline std::string WideToUtf8(const std::wstring& s)
{
    if (s.empty())
    {
        return std::string();
    }

    const int inChars = static_cast<int>(s.size());
    const int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, s.data(), inChars, nullptr, 0, nullptr, nullptr);
    if (bytesNeeded <= 0)
    {
        return std::string();
    }

    std::string out;
    out.resize(static_cast<std::size_t>(bytesNeeded));
    int written = WideCharToMultiByte(CP_UTF8, 0, s.data(), inChars, out.data(), bytesNeeded, nullptr, nullptr);
    if (written <= 0)
    {
        return std::string();
    }
    if (written != bytesNeeded)
    {
        out.resize(static_cast<std::size_t>(written));
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
    os << '"' << er2::JsonEscape(s) << '"';
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
    std::ostringstream os;

    const std::string procName = detail_metadata_hint::WideToUtf8(hint.processName);
    const std::string modName = detail_metadata_hint::WideToUtf8(hint.moduleName);
    const std::string modPath = detail_metadata_hint::WideToUtf8(hint.modulePath);

    os << "{\n";
    os << "  \"schema\": " << hint.schema << ",\n";

    os << "  \"process\": {\n";
    os << "    \"name\": ";
    detail_metadata_hint::WriteJsonStringOrNull(os, procName);
    os << ",\n";
    os << "    \"pid\": " << static_cast<std::uint64_t>(hint.pid) << "\n";
    os << "  },\n";

    os << "  \"module\": {\n";
    os << "    \"name\": ";
    detail_metadata_hint::WriteJsonStringOrNull(os, modName);
    os << ",\n";
    os << "    \"base\": \"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(hint.moduleBase), 16) << "\",\n";
    os << "    \"size\": \"0x" << std::uppercase << std::hex << hint.moduleSize << std::dec << "\",\n";
    os << "    \"path\": ";
    detail_metadata_hint::WriteJsonStringOrNull(os, modPath);
    os << ",\n";
    os << "    \"pe_image_base\": ";
    if (hint.peImageBase)
    {
        os << "\"" << detail_metadata_hint::HexU64NoPad(hint.peImageBase) << "\"\n";
    }
    else
    {
        os << "null\n";
    }
    os << "  },\n";

    os << "  \"metadata\": {\n";
    os << "    \"s_global_metadata_addr\": \"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(hint.sGlobalMetadataAddr), 16) << "\",\n";
    os << "    \"meta_base\": \"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(hint.metaBase), 16) << "\",\n";
    os << "    \"total_size\": " << static_cast<std::uint64_t>(hint.totalSize) << ",\n";
    os << "    \"magic\": \"0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << hint.magic << std::dec << "\",\n";
    os << "    \"version\": " << static_cast<std::uint64_t>(hint.version) << ",\n";

    os << "    \"images_count\": ";
    if (hint.imagesCount)
    {
        os << std::to_string(static_cast<std::uint64_t>(hint.imagesCount));
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"assemblies_count\": ";
    if (hint.assembliesCount)
    {
        os << std::to_string(static_cast<std::uint64_t>(hint.assembliesCount));
    }
    else
    {
        os << "null";
    }
    os << "\n";

    os << "  },\n";

    os << "  \"il2cpp\": {\n";

    os << "    \"code_registration\": ";
    if (hint.codeRegistration)
    {
        os << "\"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(hint.codeRegistration), 16) << "\"";
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"metadata_registration\": ";
    if (hint.metadataRegistration)
    {
        os << "\"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(hint.metadataRegistration), 16) << "\"";
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"code_registration_rva\": ";
    if (hint.codeRegistrationRva)
    {
        os << "\"" << detail_metadata_hint::HexU64NoPad(hint.codeRegistrationRva) << "\"";
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"metadata_registration_rva\": ";
    if (hint.metadataRegistrationRva)
    {
        os << "\"" << detail_metadata_hint::HexU64NoPad(hint.metadataRegistrationRva) << "\"";
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"code_gen_modules_count\": ";
    if (hint.codeGenModulesCount)
    {
        os << std::to_string(static_cast<std::uint64_t>(hint.codeGenModulesCount));
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"code_gen_modules\": ";
    if (hint.codeGenModules)
    {
        os << "\"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(hint.codeGenModules), 16) << "\"";
    }
    else
    {
        os << "null";
    }
    os << ",\n";

    os << "    \"code_gen_module_list\": [\n";
    for (std::size_t i = 0; i < hint.codeGenModuleList.size(); ++i)
    {
        const auto& m = hint.codeGenModuleList[i];
        os << "      {\n";
        os << "        \"name\": ";
        detail_metadata_hint::WriteJsonStringOrNull(os, m.name);
        os << ",\n";
        os << "        \"address\": ";
        if (m.address)
        {
            os << "\"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(m.address), 16) << "\"";
        }
        else
        {
            os << "null";
        }
        os << ",\n";
        os << "        \"method_pointer_count\": " << static_cast<std::uint64_t>(m.methodPointerCount) << ",\n";
        os << "        \"method_pointers\": ";
        if (m.methodPointers)
        {
            os << "\"" << detail_metadata_hint::HexU64(static_cast<std::uint64_t>(m.methodPointers), 16) << "\"";
        }
        else
        {
            os << "null";
        }
        os << "\n";
        os << "      }";
        if (i + 1 < hint.codeGenModuleList.size())
        {
            os << ",";
        }
        os << "\n";
    }
    os << "    ]\n";

    os << "  }\n";
    os << "}\n";

    return os.str();
}

}
