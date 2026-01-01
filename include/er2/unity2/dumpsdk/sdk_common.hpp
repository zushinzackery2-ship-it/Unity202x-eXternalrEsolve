#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace er2
{

inline std::uint32_t ReadU32LEBytes(const std::vector<std::uint8_t>& b, std::size_t off)
{
    if (off + 4 > b.size())
    {
        return 0;
    }
    return (std::uint32_t)b[off + 0] |
           ((std::uint32_t)b[off + 1] << 8) |
           ((std::uint32_t)b[off + 2] << 16) |
           ((std::uint32_t)b[off + 3] << 24);
}

inline std::int32_t ReadI32LEBytes(const std::vector<std::uint8_t>& b, std::size_t off)
{
    return static_cast<std::int32_t>(ReadU32LEBytes(b, off));
}

inline std::uint16_t ReadU16LEBytes(const std::vector<std::uint8_t>& b, std::size_t off)
{
    if (off + 2 > b.size())
    {
        return 0;
    }
    return (std::uint16_t)b[off + 0] | ((std::uint16_t)b[off + 1] << 8);
}

#pragma pack(push, 1)
struct DumpSdk6TypeDefRaw
{
    std::int32_t nameIndex;
    std::int32_t namespaceIndex;
    std::int32_t byvalTypeIndex;
    std::int32_t declaringTypeIndex;
    std::int32_t parentIndex;
    std::int32_t elementTypeIndex;
    std::int32_t genericContainerIndex;
    std::uint32_t flags;
    std::int32_t fieldStart;
    std::int32_t methodStart;
    std::int32_t eventStart;
    std::int32_t propertyStart;
    std::int32_t nestedTypesStart;
    std::int32_t interfacesStart;
    std::int32_t vtableStart;
    std::int32_t interfaceOffsetsStart;
    std::uint16_t methodCount;
    std::uint16_t propertyCount;
    std::uint16_t fieldCount;
    std::uint16_t eventCount;
    std::uint16_t nestedTypeCount;
    std::uint16_t vtableCount;
    std::uint16_t interfacesCount;
    std::uint16_t interfaceOffsetsCount;
    std::uint32_t bitfield;
    std::uint32_t token;
};

struct DumpSdk6FieldDefRaw
{
    std::int32_t nameIndex;
    std::int32_t typeIndex;
    std::uint32_t token;
};

struct DumpSdk6ParamDefRaw
{
    std::int32_t nameIndex;
    std::uint32_t token;
    std::int32_t typeIndex;
};

struct DumpSdk6GenericContainerRaw
{
    std::int32_t owner;
    std::int32_t typeArgc;
    std::int32_t isMethod;
    std::int32_t paramStart;
};
#pragma pack(pop)

struct DumpSdk6GenericParamInfo
{
    std::string name;
};

struct DumpSdk6MethodDefLayout
{
    std::size_t stride = 0;
    std::size_t tokenOffset = 0;
    std::size_t returnTypeOffset = 0;
    std::size_t parameterStartOffset = 0;
    std::size_t genericContainerOffset = 0;
    std::size_t flagsOffset = 0;
    std::size_t parameterCountOffset = 0;
};

struct DumpSdk6MethodDefFull
{
    std::int32_t nameIndex = -1;
    std::int32_t declaringType = -1;
    std::int32_t returnType = -1;
    std::int32_t parameterStart = 0;
    std::int32_t genericContainerIndex = -1;
    std::uint32_t token = 0;
    std::uint16_t flags = 0;
    std::uint16_t iflags = 0;
    std::uint16_t slot = 0;
    std::uint16_t parameterCount = 0;
};

} // namespace er2
