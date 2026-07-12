#include <er2/unity2/dumpsdk/offline/BinaryStream.h>

#include <cstring>

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
