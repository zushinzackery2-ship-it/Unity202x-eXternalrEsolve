#include "SmokeMetadataInternal.h"

#include <cstring>

namespace OfflineBehavior
{

namespace
{

struct TableSpan
{
    uint32_t offset = 0;
    int32_t size = 0;
};

constexpr size_t kMinStringBlobSize = 0x12000;
constexpr size_t kHeaderReserve = 0x110;

} // namespace

MetadataBlob BuildMetadata()
{
    StringTable strings;
    SmokeTables tables;
    BuildSmokeTables(strings, tables);

    while (strings.blob.size() < kMinStringBlobSize)
    {
        strings.blob.push_back(0);
    }

    Writer body;
    body.bytes.assign(kHeaderReserve, 0);

    auto place = [&](const std::vector<uint8_t>& blob) -> TableSpan
    {
        while (body.bytes.size() % 4 != 0)
        {
            body.bytes.push_back(0);
        }
        TableSpan span{};
        span.offset = body.Position();
        span.size = static_cast<int32_t>(blob.size());
        body.bytes.insert(body.bytes.end(), blob.begin(), blob.end());
        return span;
    };

    const TableSpan stringLiterals = place(tables.stringLiterals);
    const TableSpan stringLiteralData = place(tables.stringLiteralData);
    const TableSpan stringBlob = place(strings.blob);
    const TableSpan events = place(tables.events);
    const TableSpan properties = place(tables.properties);
    const TableSpan methods = place(tables.methods);
    const TableSpan paramDefaults = place(tables.paramDefaults);
    const TableSpan fieldDefaults = place(tables.fieldDefaults);
    const TableSpan defaultData = place(tables.defaultData);
    const TableSpan params = place(tables.params);
    const TableSpan fields = place(tables.fields);
    const TableSpan genericParameters = place(tables.genericParameters);
    const TableSpan genericContainers = place(tables.genericContainers);
    const TableSpan nestedTypes = place(tables.nestedTypes);
    const TableSpan interfaces = place(tables.interfaces);
    const TableSpan vtableMethods = place(tables.vtableMethods);
    const TableSpan typeDefs = place(tables.typeDefs);
    const TableSpan images = place(tables.images);
    const TableSpan assemblies = place(tables.assemblies);
    const TableSpan attributeData = place(tables.attributeData);
    const TableSpan attributeDataRanges = place(tables.attributeDataRanges);

    Writer header;
    header.U32(0xFAB11BAFu);
    header.I32(29);
    auto pair = [&](const TableSpan& span)
    {
        header.U32(span.offset);
        header.I32(span.size);
    };
    auto empty = [&]()
    {
        header.U32(0);
        header.I32(0);
    };

    pair(stringLiterals);
    pair(stringLiteralData);
    pair(stringBlob);
    pair(events);
    pair(properties);
    pair(methods);
    pair(paramDefaults);
    pair(fieldDefaults);
    pair(defaultData);
    empty();
    pair(params);
    pair(fields);
    pair(genericParameters);
    empty();
    pair(genericContainers);
    pair(nestedTypes);
    pair(interfaces);
    pair(vtableMethods);
    empty();
    pair(typeDefs);
    pair(images);
    pair(assemblies);
    empty();
    empty();
    pair(attributeData);
    pair(attributeDataRanges);
    empty();
    empty();
    empty();
    empty();
    empty();

    std::memcpy(body.bytes.data(), header.bytes.data(), header.bytes.size());

    MetadataBlob blob{};
    blob.bytes = std::move(body.bytes);
    blob.typeDefinitionsOffset = typeDefs.offset;
    blob.genericParametersOffset = genericParameters.offset;
    blob.typeDefCount = kTypeCount;
    blob.methodCount = kMethodCount;
    blob.imageCount = 1;
    return blob;
}

} // namespace OfflineBehavior
