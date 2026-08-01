#pragma once

#include <er2/unity2/dumpsdk/offline/DefaultValueDecoder.h>

#include <string>
#include <vector>

namespace er2
{

class CustomAttributeReader
{
public:
    CustomAttributeReader(
        const Metadata& metadata,
        const OfflineRuntimeContext& context,
        const TypeNameResolver& resolver,
        const DefaultValueDecoder& decoder);

    std::vector<std::string> Render(
        size_t imageIndex,
        int32_t customAttributeIndex,
        uint32_t token) const;

private:
    std::vector<std::string> RenderPre29(int32_t attributeIndex) const;
    std::vector<std::string> RenderPost29(int32_t attributeIndex) const;

    const Metadata& metadata_;
    const OfflineRuntimeContext& context_;
    const TypeNameResolver& resolver_;
    const DefaultValueDecoder& decoder_;
};

} // namespace er2
