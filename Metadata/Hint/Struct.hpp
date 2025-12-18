#pragma once

#include <windows.h>

#include <cstdint>
#include <string>

namespace UnityExternal
{

struct MetadataHint
{
    std::uint32_t schema = 1;

    DWORD pid = 0;
    std::wstring processName;

    std::wstring moduleName;
    std::wstring modulePath;
    std::uintptr_t moduleBase = 0;
    std::uint32_t moduleSize = 0;
    std::uint64_t peImageBase = 0;

    std::uintptr_t sGlobalMetadataAddr = 0;
    std::uintptr_t metaBase = 0;
    std::uint32_t totalSize = 0;
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t imagesCount = 0;
    std::uint32_t assembliesCount = 0;

    std::uintptr_t codeRegistration = 0;
    std::uintptr_t metadataRegistration = 0;
    std::uint64_t codeRegistrationRva = 0;
    std::uint64_t metadataRegistrationRva = 0;
};

}
