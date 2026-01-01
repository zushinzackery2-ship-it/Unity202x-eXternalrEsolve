#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../../mem/memory_read.hpp"

namespace er2
{

struct MetadataHeaderFields
{
    // header[0x04]
    std::uint32_t version = 0;

    // header[0x18..] Il2CppGlobalMetadataHeader::stringOffset/stringSize
    std::uint32_t stringOffset = 0;
    std::uint32_t stringSize = 0;

    // header[0x30..] Il2CppGlobalMetadataHeader::methodsOffset/methodsSize
    std::uint32_t methodsOffset = 0;
    std::uint32_t methodsSize = 0;

    // header[0x58..] Il2CppGlobalMetadataHeader::parametersOffset/parametersSize
    std::uint32_t parametersOffset = 0;
    std::uint32_t parametersSize = 0;

    // header[0x60..] Il2CppGlobalMetadataHeader::fieldsOffset/fieldsSize
    std::uint32_t fieldsOffset = 0;
    std::uint32_t fieldsSize = 0;

    // header[0x68..] genericParametersOffset/Size
    std::uint32_t genericParametersOffset = 0;
    std::uint32_t genericParametersSize = 0;

    // header[0x78..] genericContainersOffset/Size
    std::uint32_t genericContainersOffset = 0;
    std::uint32_t genericContainersSize = 0;

    // header[0x88..] interfacesOffset/Size
    std::uint32_t interfacesOffset = 0;
    std::uint32_t interfacesSize = 0;

    // header[0xA0..]
    std::uint32_t typeDefinitionsOffset = 0;
    std::uint32_t typeDefinitionsSize = 0;

    // header[0xA8..]
    std::uint32_t imagesOffset = 0;
    std::uint32_t imagesSize = 0;
};

namespace detail_metadata_header
{

inline std::uint32_t ReadU32LE(const std::uint8_t* p)
{
    return (std::uint32_t)p[0] |
           ((std::uint32_t)p[1] << 8) |
           ((std::uint32_t)p[2] << 16) |
           ((std::uint32_t)p[3] << 24);
}

inline std::int32_t ReadI32LE(const std::uint8_t* p)
{
    return static_cast<std::int32_t>(ReadU32LE(p));
}

inline bool ReadI32FromMem(const IMemoryAccessor& mem, std::uintptr_t addr, std::int32_t& out)
{
    return ReadValue(mem, addr, out);
}

inline std::uint32_t ToPositiveU32(std::int32_t v)
{
    return (v > 0) ? static_cast<std::uint32_t>(v) : 0u;
}

} // namespace detail_metadata_header

inline bool ReadMetadataHeaderFieldsFromMemory(const IMemoryAccessor& mem, std::uintptr_t metaBase, MetadataHeaderFields& out)
{
    out = MetadataHeaderFields{};
    if (!metaBase)
    {
        return false;
    }

    std::int32_t versionI = 0;
    std::int32_t stringOffsetI = 0;
    std::int32_t stringSizeI = 0;
    std::int32_t methodsOffsetI = 0;
    std::int32_t methodsSizeI = 0;
    std::int32_t parametersOffsetI = 0;
    std::int32_t parametersSizeI = 0;
    std::int32_t fieldsOffsetI = 0;
    std::int32_t fieldsSizeI = 0;
    std::int32_t genericParametersOffsetI = 0;
    std::int32_t genericParametersSizeI = 0;
    std::int32_t genericContainersOffsetI = 0;
    std::int32_t genericContainersSizeI = 0;
    std::int32_t interfacesOffsetI = 0;
    std::int32_t interfacesSizeI = 0;
    std::int32_t typeDefsOffsetI = 0;
    std::int32_t typeDefsSizeI = 0;
    std::int32_t imagesOffsetI = 0;
    std::int32_t imagesSizeI = 0;

    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x04u, versionI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x18u, stringOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x1Cu, stringSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x30u, methodsOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x34u, methodsSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x58u, parametersOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x5Cu, parametersSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x60u, fieldsOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x64u, fieldsSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x68u, genericParametersOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x6Cu, genericParametersSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x78u, genericContainersOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x7Cu, genericContainersSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x88u, interfacesOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0x8Cu, interfacesSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0xA0u, typeDefsOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0xA4u, typeDefsSizeI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0xA8u, imagesOffsetI)) return false;
    if (!detail_metadata_header::ReadI32FromMem(mem, metaBase + 0xACu, imagesSizeI)) return false;

    out.version = static_cast<std::uint32_t>(versionI);
    out.stringOffset = detail_metadata_header::ToPositiveU32(stringOffsetI);
    out.stringSize = detail_metadata_header::ToPositiveU32(stringSizeI);
    out.methodsOffset = detail_metadata_header::ToPositiveU32(methodsOffsetI);
    out.methodsSize = detail_metadata_header::ToPositiveU32(methodsSizeI);
    out.parametersOffset = detail_metadata_header::ToPositiveU32(parametersOffsetI);
    out.parametersSize = detail_metadata_header::ToPositiveU32(parametersSizeI);
    out.fieldsOffset = detail_metadata_header::ToPositiveU32(fieldsOffsetI);
    out.fieldsSize = detail_metadata_header::ToPositiveU32(fieldsSizeI);
    out.genericParametersOffset = detail_metadata_header::ToPositiveU32(genericParametersOffsetI);
    out.genericParametersSize = detail_metadata_header::ToPositiveU32(genericParametersSizeI);
    out.genericContainersOffset = detail_metadata_header::ToPositiveU32(genericContainersOffsetI);
    out.genericContainersSize = detail_metadata_header::ToPositiveU32(genericContainersSizeI);
    out.interfacesOffset = detail_metadata_header::ToPositiveU32(interfacesOffsetI);
    out.interfacesSize = detail_metadata_header::ToPositiveU32(interfacesSizeI);
    out.typeDefinitionsOffset = detail_metadata_header::ToPositiveU32(typeDefsOffsetI);
    out.typeDefinitionsSize = detail_metadata_header::ToPositiveU32(typeDefsSizeI);
    out.imagesOffset = detail_metadata_header::ToPositiveU32(imagesOffsetI);
    out.imagesSize = detail_metadata_header::ToPositiveU32(imagesSizeI);

    return out.version != 0 && out.stringOffset != 0 && out.stringSize != 0;
}

inline bool ReadMetadataHeaderFieldsFromBytes(const std::vector<std::uint8_t>& meta, MetadataHeaderFields& out)
{
    out = MetadataHeaderFields{};
    if (meta.size() < 0x120u)
    {
        return false;
    }

    const std::uint8_t* b = meta.data();

    const std::int32_t versionI = detail_metadata_header::ReadI32LE(b + 0x04u);
    const std::int32_t stringOffsetI = detail_metadata_header::ReadI32LE(b + 0x18u);
    const std::int32_t stringSizeI = detail_metadata_header::ReadI32LE(b + 0x1Cu);
    const std::int32_t methodsOffsetI = detail_metadata_header::ReadI32LE(b + 0x30u);
    const std::int32_t methodsSizeI = detail_metadata_header::ReadI32LE(b + 0x34u);
    const std::int32_t parametersOffsetI = detail_metadata_header::ReadI32LE(b + 0x58u);
    const std::int32_t parametersSizeI = detail_metadata_header::ReadI32LE(b + 0x5Cu);
    const std::int32_t fieldsOffsetI = detail_metadata_header::ReadI32LE(b + 0x60u);
    const std::int32_t fieldsSizeI = detail_metadata_header::ReadI32LE(b + 0x64u);
    const std::int32_t genericParametersOffsetI = detail_metadata_header::ReadI32LE(b + 0x68u);
    const std::int32_t genericParametersSizeI = detail_metadata_header::ReadI32LE(b + 0x6Cu);
    const std::int32_t genericContainersOffsetI = detail_metadata_header::ReadI32LE(b + 0x78u);
    const std::int32_t genericContainersSizeI = detail_metadata_header::ReadI32LE(b + 0x7Cu);
    const std::int32_t interfacesOffsetI = detail_metadata_header::ReadI32LE(b + 0x88u);
    const std::int32_t interfacesSizeI = detail_metadata_header::ReadI32LE(b + 0x8Cu);
    const std::int32_t typeDefsOffsetI = detail_metadata_header::ReadI32LE(b + 0xA0u);
    const std::int32_t typeDefsSizeI = detail_metadata_header::ReadI32LE(b + 0xA4u);
    const std::int32_t imagesOffsetI = detail_metadata_header::ReadI32LE(b + 0xA8u);
    const std::int32_t imagesSizeI = detail_metadata_header::ReadI32LE(b + 0xACu);

    out.version = static_cast<std::uint32_t>(versionI);
    out.stringOffset = detail_metadata_header::ToPositiveU32(stringOffsetI);
    out.stringSize = detail_metadata_header::ToPositiveU32(stringSizeI);
    out.methodsOffset = detail_metadata_header::ToPositiveU32(methodsOffsetI);
    out.methodsSize = detail_metadata_header::ToPositiveU32(methodsSizeI);
    out.parametersOffset = detail_metadata_header::ToPositiveU32(parametersOffsetI);
    out.parametersSize = detail_metadata_header::ToPositiveU32(parametersSizeI);
    out.fieldsOffset = detail_metadata_header::ToPositiveU32(fieldsOffsetI);
    out.fieldsSize = detail_metadata_header::ToPositiveU32(fieldsSizeI);
    out.genericParametersOffset = detail_metadata_header::ToPositiveU32(genericParametersOffsetI);
    out.genericParametersSize = detail_metadata_header::ToPositiveU32(genericParametersSizeI);
    out.genericContainersOffset = detail_metadata_header::ToPositiveU32(genericContainersOffsetI);
    out.genericContainersSize = detail_metadata_header::ToPositiveU32(genericContainersSizeI);
    out.interfacesOffset = detail_metadata_header::ToPositiveU32(interfacesOffsetI);
    out.interfacesSize = detail_metadata_header::ToPositiveU32(interfacesSizeI);
    out.typeDefinitionsOffset = detail_metadata_header::ToPositiveU32(typeDefsOffsetI);
    out.typeDefinitionsSize = detail_metadata_header::ToPositiveU32(typeDefsSizeI);
    out.imagesOffset = detail_metadata_header::ToPositiveU32(imagesOffsetI);
    out.imagesSize = detail_metadata_header::ToPositiveU32(imagesSizeI);

    return out.version != 0 && out.stringOffset != 0 && out.stringSize != 0;
}

} // namespace er2
