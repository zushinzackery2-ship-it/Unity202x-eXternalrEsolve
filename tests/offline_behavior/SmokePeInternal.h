#pragma once

#include "SmokePeBuilder.h"

#include <cstring>
#include <vector>

namespace OfflineBehavior
{

constexpr uint32_t kHeadersSize = 0x400;
constexpr uint32_t kTextRva = 0x1000;
constexpr uint32_t kTextSize = 0x1000;
constexpr uint32_t kTextFileOffset = 0x400;
constexpr uint32_t kDataRva = 0x2000;
constexpr uint32_t kDataSize = 0x3000;
constexpr uint32_t kDataFileOffset = 0x1400;
constexpr uint32_t kImageSize = 0x5000;
constexpr uint32_t kMethodStride = 0x20;
constexpr uint32_t kPtr = 8;
constexpr uint32_t kFieldOffsetStride = 32;

class DataSection
{
public:
    DataSection(uint8_t* image, uint64_t imageBase)
        : image_(image)
        , imageBase_(imageBase)
    {
    }

    uint32_t Reserve(size_t bytes)
    {
        cursor_ = (cursor_ + 7u) & ~7u;
        const uint32_t offset = cursor_;
        cursor_ += static_cast<uint32_t>(bytes);
        return offset;
    }

    uint64_t Va(uint32_t offset) const
    {
        return imageBase_ + kDataRva + offset;
    }

    void PutU64(uint32_t offset, uint64_t value)
    {
        std::memcpy(image_ + kDataRva + offset, &value, sizeof(value));
    }

    void PutU32(uint32_t offset, uint32_t value)
    {
        std::memcpy(image_ + kDataRva + offset, &value, sizeof(value));
    }

    void PutI32(uint32_t offset, int32_t value)
    {
        std::memcpy(image_ + kDataRva + offset, &value, sizeof(value));
    }

    void PutText(uint32_t offset, const char* value)
    {
        std::memcpy(image_ + kDataRva + offset, value, std::strlen(value) + 1);
    }

    uint32_t Used() const
    {
        return cursor_;
    }

private:
    uint8_t* image_ = nullptr;
    uint64_t imageBase_ = 0;
    uint32_t cursor_ = 0;
};

struct RuntimeTypeSpec
{
    uint32_t typeEnum = 0;
    uint32_t attrs = 0;
    uint32_t byref = 0;
    int32_t typeDefIndex = -1;
    int32_t genericParameterIndex = -1;
    int32_t elementRuntimeType = -1;
};

std::vector<RuntimeTypeSpec> BuildRuntimeTypeSpecs();
void WritePeHeaders(uint8_t* image, uint64_t imageBase);

} // namespace OfflineBehavior
