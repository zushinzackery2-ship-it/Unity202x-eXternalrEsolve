#include "GlobalStringDecoder.h"

#include <algorithm>

namespace er2
{

namespace
{

bool IsControlCodePoint(std::uint32_t codePoint)
{
    return (codePoint < 0x20 && codePoint != '\t' && codePoint != '\r' && codePoint != '\n')
        || (codePoint >= 0x7F && codePoint <= 0x9F);
}

bool DecodeUtf8CodePoint(
    const std::uint8_t* bytes,
    std::size_t length,
    std::size_t& offset,
    std::uint32_t& codePoint)
{
    const std::uint8_t first = bytes[offset++];
    if (first < 0x80)
    {
        codePoint = first;
        return true;
    }

    std::size_t continuationCount = 0;
    std::uint32_t minimum = 0;
    if ((first & 0xE0) == 0xC0)
    {
        continuationCount = 1;
        codePoint = first & 0x1F;
        minimum = 0x80;
    }
    else if ((first & 0xF0) == 0xE0)
    {
        continuationCount = 2;
        codePoint = first & 0x0F;
        minimum = 0x800;
    }
    else if ((first & 0xF8) == 0xF0)
    {
        continuationCount = 3;
        codePoint = first & 0x07;
        minimum = 0x10000;
    }
    else
    {
        return false;
    }

    if (continuationCount > length - offset)
    {
        return false;
    }
    for (std::size_t index = 0; index < continuationCount; ++index)
    {
        const std::uint8_t next = bytes[offset++];
        if ((next & 0xC0) != 0x80)
        {
            return false;
        }
        codePoint = (codePoint << 6) | (next & 0x3F);
    }

    return codePoint >= minimum
        && codePoint <= 0x10FFFF
        && !(codePoint >= 0xD800 && codePoint <= 0xDFFF);
}

} // namespace

bool GlobalStringDecoder::TryDecode(
    const std::uint8_t* bytes,
    std::size_t available,
    const GlobalStringXrefOptions& options,
    DecodedGlobalString& decoded)
{
    decoded = {};
    return TryDecodeUtf8(bytes, available, options, decoded)
        || TryDecodeUtf16(bytes, available, options, decoded);
}

bool GlobalStringDecoder::TryDecodeUtf8(
    const std::uint8_t* bytes,
    std::size_t available,
    const GlobalStringXrefOptions& options,
    DecodedGlobalString& decoded)
{
    const std::size_t limit = std::min(available, options.maximumByteLength + 1);
    std::size_t byteLength = 0;
    while (byteLength < limit && bytes[byteLength] != 0)
    {
        ++byteLength;
    }
    if (byteLength >= limit || byteLength >= options.maximumByteLength)
    {
        return false;
    }

    std::size_t offset = 0;
    std::size_t characterCount = 0;
    while (offset < byteLength)
    {
        std::uint32_t codePoint = 0;
        if (!DecodeUtf8CodePoint(bytes, byteLength, offset, codePoint) || IsControlCodePoint(codePoint))
        {
            return false;
        }
        ++characterCount;
    }
    if (characterCount < options.minimumLength)
    {
        return false;
    }

    decoded.value.assign(reinterpret_cast<const char*>(bytes), byteLength);
    decoded.encoding = "utf-8";
    decoded.byteLength = byteLength + 1;
    return true;
}

bool GlobalStringDecoder::TryDecodeUtf16(
    const std::uint8_t* bytes,
    std::size_t available,
    const GlobalStringXrefOptions& options,
    DecodedGlobalString& decoded)
{
    const std::size_t limit = std::min(available, options.maximumByteLength);
    std::size_t byteLength = 0;
    while (byteLength + 1 < limit && (bytes[byteLength] != 0 || bytes[byteLength + 1] != 0))
    {
        byteLength += 2;
    }
    if (byteLength + 1 >= limit || byteLength < options.minimumLength * 2)
    {
        return false;
    }

    std::string value;
    value.reserve(byteLength / 2);
    for (std::size_t offset = 0; offset < byteLength; offset += 2)
    {
        const std::uint16_t character = static_cast<std::uint16_t>(bytes[offset])
            | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
        if (character > 0x7E
            || (character < 0x20 && character != '\t' && character != '\r' && character != '\n'))
        {
            return false;
        }
        value.push_back(static_cast<char>(character));
    }

    decoded.value = std::move(value);
    decoded.encoding = "utf-16le";
    decoded.byteLength = byteLength + 2;
    return true;
}

} // namespace er2
