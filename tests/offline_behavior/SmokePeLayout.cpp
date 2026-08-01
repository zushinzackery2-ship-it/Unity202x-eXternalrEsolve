#include "SmokePeInternal.h"

namespace OfflineBehavior
{

namespace
{

constexpr uint32_t kEnumVoid = 0x01;
constexpr uint32_t kEnumI4 = 0x08;
constexpr uint32_t kEnumString = 0x0E;
constexpr uint32_t kEnumClass = 0x12;
constexpr uint32_t kEnumVar = 0x13;
constexpr uint32_t kEnumArray = 0x14;
constexpr uint32_t kEnumGenericInst = 0x15;
constexpr uint32_t kEnumObject = 0x1C;
constexpr uint32_t kEnumSzArray = 0x1D;
constexpr uint32_t kEnumMVar = 0x1E;

constexpr uint32_t kFieldPrivate = 0x0001;
constexpr uint32_t kFieldPublic = 0x0006;
constexpr uint32_t kFieldStatic = 0x0010;
constexpr uint32_t kFieldInitOnly = 0x0020;
constexpr uint32_t kFieldLiteral = 0x0040;
constexpr uint32_t kParamIn = 0x0001;
constexpr uint32_t kParamOut = 0x0002;

} // namespace

std::vector<RuntimeTypeSpec> BuildRuntimeTypeSpecs()
{
    std::vector<RuntimeTypeSpec> types(kRtCount);
    auto classOf = [&](SmokeRuntimeType slot, SmokeTypeIndex typeDef)
    {
        types[slot].typeEnum = kEnumClass;
        types[slot].typeDefIndex = typeDef;
    };

    types[kRtObject].typeEnum = kEnumObject;
    types[kRtObject].typeDefIndex = kTypeObject;
    classOf(kRtValueType, kTypeValueType);
    classOf(kRtEnum, kTypeEnum);
    types[kRtInt32].typeEnum = kEnumI4;
    types[kRtInt32].typeDefIndex = kTypeInt32;
    types[kRtString].typeEnum = kEnumString;
    types[kRtString].typeDefIndex = kTypeString;
    classOf(kRtIDisposable, kTypeIDisposable);
    classOf(kRtBase, kTypeBase);
    classOf(kRtDerived, kTypeDerived);
    classOf(kRtNested, kTypeNested);
    classOf(kRtGeneric, kTypeGeneric);
    classOf(kRtAttribute, kTypeAttribute);

    types[kRtVoid].typeEnum = kEnumVoid;
    types[kRtVar0].typeEnum = kEnumVar;
    types[kRtVar0].genericParameterIndex = 0;
    types[kRtMVar0].typeEnum = kEnumMVar;
    types[kRtMVar0].genericParameterIndex = 1;

    types[kRtFieldCounter].typeEnum = kEnumI4;
    types[kRtFieldCounter].attrs = kFieldPrivate;
    types[kRtFieldTag].typeEnum = kEnumString;
    types[kRtFieldTag].attrs = kFieldPublic | kFieldInitOnly;
    types[kRtFieldAnswer].typeEnum = kEnumI4;
    types[kRtFieldAnswer].attrs = kFieldPublic | kFieldStatic | kFieldLiteral;
    types[kRtFieldValues].typeEnum = kEnumSzArray;
    types[kRtFieldValues].attrs = kFieldPublic;
    types[kRtFieldValues].elementRuntimeType = kRtInt32;
    types[kRtFieldMatrix].typeEnum = kEnumArray;
    types[kRtFieldMatrix].attrs = kFieldPublic;
    types[kRtFieldGeneric].typeEnum = kEnumGenericInst;
    types[kRtFieldGeneric].attrs = kFieldPublic;
    types[kRtFieldNoOffset].typeEnum = kEnumI4;
    types[kRtFieldNoOffset].attrs = kFieldPublic;

    types[kRtParamOutInt].typeEnum = kEnumI4;
    types[kRtParamOutInt].attrs = kParamOut;
    types[kRtParamOutInt].byref = 1;
    types[kRtParamRefInt].typeEnum = kEnumI4;
    types[kRtParamRefInt].byref = 1;
    types[kRtParamInInt].typeEnum = kEnumI4;
    types[kRtParamInInt].attrs = kParamIn;
    types[kRtParamInInt].byref = 1;
    return types;
}

void WritePeHeaders(uint8_t* image, uint64_t imageBase)
{
    constexpr uint32_t kLfanew = 0x80;
    constexpr uint16_t kOptionalHeaderSize = 240;

    auto put16 = [&](uint32_t offset, uint16_t value)
    {
        std::memcpy(image + offset, &value, sizeof(value));
    };
    auto put32 = [&](uint32_t offset, uint32_t value)
    {
        std::memcpy(image + offset, &value, sizeof(value));
    };
    auto put64 = [&](uint32_t offset, uint64_t value)
    {
        std::memcpy(image + offset, &value, sizeof(value));
    };

    put16(0, 0x5A4D);
    put32(0x3C, kLfanew);
    put32(kLfanew, 0x00004550);
    const uint32_t fileHeader = kLfanew + 4;
    put16(fileHeader + 0, 0x8664);
    put16(fileHeader + 2, 2);
    put16(fileHeader + 16, kOptionalHeaderSize);
    put16(fileHeader + 18, 0x2022);

    const uint32_t optional = fileHeader + 20;
    put16(optional + 0, 0x020B);
    put32(optional + 4, kTextSize);
    put32(optional + 8, kDataSize);
    put32(optional + 20, kTextRva);
    put64(optional + 24, imageBase);
    put32(optional + 32, 0x1000);
    put32(optional + 36, 0x200);
    put32(optional + 56, kImageSize);
    put32(optional + 60, kHeadersSize);
    put16(optional + 68, 3);
    put32(optional + 108, 16);

    const uint32_t sections = optional + kOptionalHeaderSize;
    auto section = [&](uint32_t slot,
        const char* name,
        uint32_t rva,
        uint32_t size,
        uint32_t fileOffset,
        uint32_t characteristics)
    {
        const uint32_t at = sections + slot * 40;
        std::memcpy(image + at, name, std::strlen(name));
        put32(at + 8, size);
        put32(at + 12, rva);
        put32(at + 16, size);
        put32(at + 20, fileOffset);
        put32(at + 36, characteristics);
    };

    section(0, ".text", kTextRva, kTextSize, kTextFileOffset, 0x60000020);
    section(1, ".data", kDataRva, kDataSize, kDataFileOffset, 0xC0000040);
}

} // namespace OfflineBehavior
