#pragma once

#include <er2/unity2/dumpsdk/offline/TypeNameResolver.h>

#include <string>

namespace er2
{

class DefaultValueDecoder
{
public:
    DefaultValueDecoder(
        const Metadata& metadata,
        const OfflineRuntimeContext& context,
        const TypeNameResolver& resolver);

    bool TryRenderDefaultValue(int32_t typeIndex, int32_t dataIndex, std::string& text) const;
    bool TryRenderAttributeValue(size_t& position, std::string& text) const;

    Il2CppTypeEnum ReadEncodedTypeEnum(
        BinaryStream& reader,
        const Il2CppTypeRuntime** enumType) const;
    bool RenderConstant(Il2CppTypeEnum type, BinaryStream& reader, std::string& text) const;

private:
    const Metadata& metadata_;
    const OfflineRuntimeContext& context_;
    const TypeNameResolver& resolver_;
};

std::string EscapeCSharpString(const std::string& value);

} // namespace er2
