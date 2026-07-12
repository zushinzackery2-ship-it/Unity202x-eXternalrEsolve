#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace er2
{

class StreamBoundsError : public std::runtime_error
{
public:
    explicit StreamBoundsError(const char* message)
        : std::runtime_error(message)
    {
    }
};

class BinaryStream
{
public:
    BinaryStream() = default;

    BinaryStream(const uint8_t* data, size_t size);

    void Bind(const uint8_t* data, size_t size);

    bool IsBound() const
    {
        return data_ != nullptr;
    }

    const uint8_t* Data() const
    {
        return data_;
    }

    size_t Size() const
    {
        return size_;
    }

    size_t Position() const
    {
        return position_;
    }

    void SetPosition(size_t position);

    void Skip(size_t bytes);

    uint8_t ReadUInt8();
    int8_t ReadInt8();
    uint16_t ReadUInt16();
    int16_t ReadInt16();
    uint32_t ReadUInt32();
    int32_t ReadInt32();
    uint64_t ReadUInt64();
    int64_t ReadInt64();

    std::vector<uint8_t> ReadBytes(size_t count);

    std::string ReadStringToNull();
    std::string ReadStringToNull(size_t address);

    template<typename T>
    T ReadValue()
    {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
            "ReadValue supports primitive integer sizes only");
        if constexpr (sizeof(T) == 1)
        {
            return static_cast<T>(ReadUInt8());
        }
        else if constexpr (sizeof(T) == 2)
        {
            return static_cast<T>(ReadUInt16());
        }
        else if constexpr (sizeof(T) == 4)
        {
            return static_cast<T>(ReadUInt32());
        }
        else
        {
            return static_cast<T>(ReadUInt64());
        }
    }

protected:
    void EnsureReadable(size_t bytes) const;

    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t position_ = 0;
};

} // namespace er2
