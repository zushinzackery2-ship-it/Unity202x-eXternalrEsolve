#include <er2/unity2/dumpsdk/offline/Metadata.h>

#include <algorithm>

namespace er2
{

void Metadata::ProcessMetadataUsage()
{
    metadataUsageDic_.clear();
    for (uint32_t kind = 1; kind <= 6; ++kind)
    {
        metadataUsageDic_[kind] = {};
    }

    for (const Il2CppMetadataUsageList& list : metadataUsageLists_)
    {
        for (uint32_t i = 0; i < list.count; ++i)
        {
            const uint64_t offset = static_cast<uint64_t>(list.start) + i;
            if (offset >= metadataUsagePairs_.size())
            {
                continue;
            }
            const Il2CppMetadataUsagePair& pair = metadataUsagePairs_[static_cast<size_t>(offset)];
            const uint32_t usage = GetEncodedIndexType(pair.encodedSourceIndex);
            if (usage < 1 || usage > 6)
            {
                continue;
            }
            metadataUsageDic_[usage][pair.destinationIndex] =
                GetDecodedMethodIndex(pair.encodedSourceIndex);
        }
    }

    int64_t maxIndex = -1;
    for (const auto& [kind, entries] : metadataUsageDic_)
    {
        (void)kind;
        for (const auto& [destination, source] : entries)
        {
            (void)source;
            maxIndex = (std::max)(maxIndex, static_cast<int64_t>(destination));
        }
    }
    metadataUsagesCount_ = maxIndex + 1;
}

uint32_t Metadata::GetEncodedIndexType(uint32_t index)
{
    return (index & 0xE0000000u) >> 29;
}

uint32_t Metadata::GetDecodedMethodIndex(uint32_t index) const
{
    if (version_ >= 27.0)
    {
        return (index & 0x1FFFFFFEu) >> 1;
    }
    return index & 0x1FFFFFFFu;
}

Il2CppFieldRef Metadata::ReadFieldRef()
{
    Il2CppFieldRef value{};
    value.typeIndex = ReadInt32();
    value.fieldIndex = ReadInt32();
    return value;
}

Il2CppMetadataUsageList Metadata::ReadMetadataUsageList()
{
    Il2CppMetadataUsageList value{};
    value.start = ReadUInt32();
    value.count = ReadUInt32();
    return value;
}

Il2CppMetadataUsagePair Metadata::ReadMetadataUsagePair()
{
    Il2CppMetadataUsagePair value{};
    value.destinationIndex = ReadUInt32();
    value.encodedSourceIndex = ReadUInt32();
    return value;
}

} // namespace er2
