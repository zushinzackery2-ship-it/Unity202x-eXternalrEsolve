#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace er2
{

struct GlobalStringXrefOptions
{
    std::size_t minimumLength = 4;
    std::size_t maximumByteLength = 4096;
};

struct DecodedGlobalString
{
    std::string value;
    std::string encoding;
    std::size_t byteLength = 0;
};

struct GlobalStringReference
{
    std::uint32_t rva = 0;
    std::uintptr_t targetAddress = 0;
    std::string section;
    std::string kind;
    std::string mnemonic;
};

struct DetectedGlobalString
{
    std::uintptr_t address = 0;
    std::uint32_t rva = 0;
    std::size_t byteLength = 0;
    std::string section;
    std::string encoding;
    std::string value;
    std::vector<GlobalStringReference> references;
};

struct GlobalStringReferenceCandidate
{
    std::uint32_t instructionRva = 0;
    std::uintptr_t targetAddress = 0;
    std::string section;
    std::string kind;
    std::string mnemonic;
};

struct GlobalStringXrefResult
{
    std::size_t stringCount = 0;
    std::size_t referenceCount = 0;
};

struct GlobalStringXrefReportResults
{
    GlobalStringXrefResult global;
    GlobalStringXrefResult runtimeRdata;
};

} // namespace er2
