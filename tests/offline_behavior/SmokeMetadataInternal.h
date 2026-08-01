#pragma once

#include "SmokeMetadataBuilder.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace OfflineBehavior
{

class Writer
{
public:
    void U8(uint8_t value)
    {
        bytes.push_back(value);
    }

    void U16(uint16_t value)
    {
        Append(&value, sizeof(value));
    }

    void U32(uint32_t value)
    {
        Append(&value, sizeof(value));
    }

    void I32(int32_t value)
    {
        Append(&value, sizeof(value));
    }

    void Text(const std::string& value)
    {
        bytes.insert(bytes.end(), value.begin(), value.end());
    }

    void CompressedI32(int32_t value)
    {
        const uint32_t encoded = value < 0
            ? (static_cast<uint32_t>(-(value + 1)) << 1) | 1u
            : static_cast<uint32_t>(value) << 1;
        CompressedU32(encoded);
    }

    void CompressedU32(uint32_t value)
    {
        if (value < 0x80u)
        {
            U8(static_cast<uint8_t>(value));
            return;
        }
        if (value < 0x4000u)
        {
            U8(static_cast<uint8_t>(0x80u | (value >> 8)));
            U8(static_cast<uint8_t>(value & 0xFFu));
            return;
        }
        U8(static_cast<uint8_t>(0xC0u | (value >> 24)));
        U8(static_cast<uint8_t>((value >> 16) & 0xFFu));
        U8(static_cast<uint8_t>((value >> 8) & 0xFFu));
        U8(static_cast<uint8_t>(value & 0xFFu));
    }

    uint32_t Position() const
    {
        return static_cast<uint32_t>(bytes.size());
    }

    std::vector<uint8_t> bytes;

private:
    void Append(const void* data, size_t size)
    {
        const uint8_t* source = static_cast<const uint8_t*>(data);
        bytes.insert(bytes.end(), source, source + size);
    }
};

class StringTable
{
public:
    uint32_t Add(const std::string& value)
    {
        const auto found = offsets_.find(value);
        if (found != offsets_.end())
        {
            return found->second;
        }
        const uint32_t offset = static_cast<uint32_t>(blob.size());
        blob.insert(blob.end(), value.begin(), value.end());
        blob.push_back(0);
        offsets_.emplace(value, offset);
        return offset;
    }

    std::vector<uint8_t> blob;

private:
    std::map<std::string, uint32_t> offsets_;
};

struct SmokeTables
{
    std::vector<uint8_t> typeDefs;
    std::vector<uint8_t> methods;
    std::vector<uint8_t> params;
    std::vector<uint8_t> fields;
    std::vector<uint8_t> properties;
    std::vector<uint8_t> events;
    std::vector<uint8_t> interfaces;
    std::vector<uint8_t> nestedTypes;
    std::vector<uint8_t> vtableMethods;
    std::vector<uint8_t> genericContainers;
    std::vector<uint8_t> genericParameters;
    std::vector<uint8_t> fieldDefaults;
    std::vector<uint8_t> paramDefaults;
    std::vector<uint8_t> defaultData;
    std::vector<uint8_t> stringLiterals;
    std::vector<uint8_t> stringLiteralData;
    std::vector<uint8_t> images;
    std::vector<uint8_t> assemblies;
    std::vector<uint8_t> attributeData;
    std::vector<uint8_t> attributeDataRanges;
};

constexpr uint32_t kTypeTokenBase = 0x02000001u;
constexpr uint32_t kMethodTokenBase = 0x06000001u;
constexpr uint32_t kFieldTokenBase = 0x04000001u;
constexpr uint32_t kParamTokenBase = 0x08000001u;
constexpr uint32_t kPropertyToken = 0x17000001u;
constexpr uint32_t kEventToken = 0x14000001u;
constexpr uint32_t kAssemblyToken = 0x20000001u;
constexpr uint32_t kImageToken = 0x00000001u;

void BuildTypeAndMethodTables(StringTable& strings, SmokeTables& tables);
void BuildSmokeTables(StringTable& strings, SmokeTables& tables);

} // namespace OfflineBehavior
