#include <er2/unity2/dumpsdk/offline/BinaryStream.h>

#include <cstring>
#include <limits>

namespace er2
{

BinaryStream::BinaryStream(const uint8_t* data, size_t size)
{
    Bind(data, size);
}

void BinaryStream::Bind(const uint8_t* data, size_t size)
{
    data_ = data;
    size_ = size;
    position_ = 0;
}

void BinaryStream::SetPosition(size_t position)
{
    if (position > size_)
    {
        throw StreamBoundsError("BinaryStream position out of range");
    }
    position_ = position;
}

void BinaryStream::Skip(size_t bytes)
{
    SetPosition(position_ + bytes);
}

void BinaryStream::EnsureReadable(size_t bytes) const
{
    if (data_ == nullptr)
    {
        throw StreamBoundsError("BinaryStream is not bound");
    }
    if (position_ > size_ || bytes > size_ - position_)
    {
        throw StreamBoundsError("BinaryStream read past end");
    }
}

uint8_t BinaryStream::ReadUInt8()
{
    EnsureReadable(1);
    return data_[position_++];
}

int8_t BinaryStream::ReadInt8()
{
    return static_cast<int8_t>(ReadUInt8());
}

uint16_t BinaryStream::ReadUInt16()
{
    EnsureReadable(2);
    uint16_t value = 0;
    std::memcpy(&value, data_ + position_, sizeof(value));
    position_ += 2;
    return value;
}

int16_t BinaryStream::ReadInt16()
{
    return static_cast<int16_t>(ReadUInt16());
}

uint32_t BinaryStream::ReadUInt32()
{
    EnsureReadable(4);
    uint32_t value = 0;
    std::memcpy(&value, data_ + position_, sizeof(value));
    position_ += 4;
    return value;
}

int32_t BinaryStream::ReadInt32()
{
    return static_cast<int32_t>(ReadUInt32());
}

uint64_t BinaryStream::ReadUInt64()
{
    EnsureReadable(8);
    uint64_t value = 0;
    std::memcpy(&value, data_ + position_, sizeof(value));
    position_ += 8;
    return value;
}

int64_t BinaryStream::ReadInt64()
{
    return static_cast<int64_t>(ReadUInt64());
}

float BinaryStream::ReadSingle()
{
    const uint32_t bits = ReadUInt32();
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

double BinaryStream::ReadDouble()
{
    const uint64_t bits = ReadUInt64();
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

uint32_t BinaryStream::ReadCompressedUInt32()
{
    const uint8_t first = ReadUInt8();
    if ((first & 0x80u) == 0)
    {
        return first;
    }
    if ((first & 0xC0u) == 0x80u)
    {
        return (static_cast<uint32_t>(first & ~0x80u) << 8) | ReadUInt8();
    }
    if ((first & 0xE0u) == 0xC0u)
    {
        const uint8_t second = ReadUInt8();
        const uint8_t third = ReadUInt8();
        const uint8_t fourth = ReadUInt8();
        return (static_cast<uint32_t>(first & ~0xC0u) << 24) |
            (static_cast<uint32_t>(second) << 16) |
            (static_cast<uint32_t>(third) << 8) |
            fourth;
    }
    if (first == 0xF0u)
    {
        return ReadUInt32();
    }
    if (first == 0xFEu)
    {
        return 0xFFFFFFFEu;
    }
    if (first == 0xFFu)
    {
        return 0xFFFFFFFFu;
    }
    throw StreamBoundsError("invalid compressed uint32 prefix");
}

int32_t BinaryStream::ReadCompressedInt32()
{
    const uint32_t encoded = ReadCompressedUInt32();
    if (encoded == 0xFFFFFFFFu)
    {
        return (std::numeric_limits<int32_t>::min)();
    }
    const uint32_t shifted = encoded >> 1;
    if ((encoded & 1u) == 0)
    {
        return static_cast<int32_t>(shifted);
    }
    return -static_cast<int32_t>(shifted + 1u);
}

std::vector<uint8_t> BinaryStream::ReadBytes(size_t count)
{
    EnsureReadable(count);
    std::vector<uint8_t> buffer(count);
    if (count > 0)
    {
        std::memcpy(buffer.data(), data_ + position_, count);
        position_ += count;
    }
    return buffer;
}

std::string BinaryStream::ReadStringToNull()
{
    std::string value;
    while (true)
    {
        const uint8_t byte = ReadUInt8();
        if (byte == 0)
        {
            break;
        }
        value.push_back(static_cast<char>(byte));
    }
    return value;
}

std::string BinaryStream::ReadStringToNull(size_t address)
{
    SetPosition(address);
    return ReadStringToNull();
}

} // namespace er2
