#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace er2
{

struct CodeGenModuleHint
{
    // Il2CppCodeGenModule::moduleName
    std::string name;

    // VA of the Il2CppCodeGenModule struct itself.
    std::uintptr_t address = 0;

    // Il2CppCodeGenModule::methodPointerCount / methodPointers
    std::uint32_t methodPointerCount = 0;
    std::uintptr_t methodPointers = 0;
};

struct MetadataHint
{
    std::uint32_t schema = 1;

    std::uint32_t pid = 0;
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

    // CodeGenModules (resolved from CodeRegistration or scanned heuristically).
    std::uint32_t codeGenModulesCount = 0;
    std::uintptr_t codeGenModules = 0; // VA of Il2CppCodeGenModule*[]
    std::vector<CodeGenModuleHint> codeGenModuleList;
};

}
