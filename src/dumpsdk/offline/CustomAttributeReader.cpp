#include <er2/unity2/dumpsdk/offline/CustomAttributeReader.h>

#include <format>

namespace er2
{

CustomAttributeReader::CustomAttributeReader(
    const Metadata& metadata,
    const OfflineRuntimeContext& context,
    const TypeNameResolver& resolver,
    const DefaultValueDecoder& decoder)
    : metadata_(metadata)
    , context_(context)
    , resolver_(resolver)
    , decoder_(decoder)
{
}

std::vector<std::string> CustomAttributeReader::Render(
    size_t imageIndex,
    int32_t customAttributeIndex,
    uint32_t token) const
{
    if (context_.Version() < 21.0)
    {
        return {};
    }
    const int32_t index = metadata_.GetCustomAttributeIndex(
        imageIndex,
        customAttributeIndex,
        token);
    if (index < 0)
    {
        return {};
    }
    return context_.Version() < 29.0 ? RenderPre29(index) : RenderPost29(index);
}

std::vector<std::string> CustomAttributeReader::RenderPre29(int32_t attributeIndex) const
{
    const std::vector<Il2CppCustomAttributeTypeRange>& ranges = metadata_.AttributeTypeRanges();
    if (static_cast<size_t>(attributeIndex) >= ranges.size())
    {
        return {};
    }

    const Il2CppCustomAttributeTypeRange& range = ranges[static_cast<size_t>(attributeIndex)];
    const uintptr_t generator = context_.GetCustomAttributeGenerator(attributeIndex);
    const uint64_t rva = generator != 0 ? generator - context_.Pe().ImageBase() : 0;
    const uint64_t fileOffset = generator != 0 ? context_.Pe().MapFileOffset(generator) : 0;

    const std::vector<int32_t>& types = metadata_.AttributeTypes();
    std::vector<std::string> result;
    for (int32_t i = 0; i < range.count; ++i)
    {
        const int64_t index = static_cast<int64_t>(range.start) + i;
        if (index < 0 || static_cast<size_t>(index) >= types.size())
        {
            continue;
        }
        const Il2CppTypeRuntime* type = context_.GetTypeByIndex(types[static_cast<size_t>(index)]);
        if (type == nullptr)
        {
            continue;
        }
        const std::string name = resolver_.GetTypeName(*type, false, false);
        if (generator == 0)
        {
            result.push_back("[" + name + "]");
        }
        else
        {
            result.push_back(std::format(
                "[{}] // RVA: 0x{:X} Offset: 0x{:X} VA: 0x{:X}",
                name,
                rva,
                fileOffset,
                generator));
        }
    }
    return result;
}

} // namespace er2
