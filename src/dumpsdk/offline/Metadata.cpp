#include <er2/unity2/dumpsdk/offline/Metadata.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>

#include <format>
#include <stdexcept>

namespace er2
{

namespace
{

class MetadataFormatError : public std::runtime_error
{
public:
    explicit MetadataFormatError(const char* message)
        : std::runtime_error(message)
    {
    }
};

constexpr uint32_t kMetadataSanity = 0xFAB11BAFu;

} // namespace

Metadata::Metadata(const uint8_t* data, size_t size)
{
    Load(data, size);
}

Metadata Metadata::CreateBoundView() const
{
    Metadata view;
    view.Bind(Data(), Size());
    view.version_ = version_;
    view.SetPosition(Position());
    return view;
}

void Metadata::Load(const uint8_t* data, size_t size)
{
    Bind(data, size);
    stringCache_.clear();
    imageDefs_.clear();
    typeDefs_.clear();
    methodDefs_.clear();
    fieldDefs_.clear();
    parameterDefs_.clear();
    propertyDefs_.clear();
    stringLiterals_.clear();
    eventDefs_.clear();
    interfaceIndices_.clear();
    genericContainers_.clear();
    genericParameters_.clear();
    fieldRefs_.clear();
    attributeTypeRanges_.clear();
    attributeTypes_.clear();
    attributeDataRanges_.clear();
    metadataUsageLists_.clear();
    metadataUsagePairs_.clear();
    metadataUsageDic_.clear();
    metadataUsagesCount_ = 0;
    fieldDefaultValues_.clear();
    parameterDefaultValues_.clear();
    attributeTokenMaps_.clear();
    Parse();
}

bool Metadata::VersionInRange(double version, double minVersion, double maxVersion, bool hasMin, bool hasMax)
{
    if (hasMin && version < minVersion)
    {
        return false;
    }
    if (hasMax && version > maxVersion)
    {
        return false;
    }
    return true;
}

void Metadata::Parse()
{
    if (Size() < 8)
    {
        throw MetadataFormatError("metadata buffer too small");
    }

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Metadata::Parse begin size={}", Size()));

    SetPosition(0);
    const uint32_t sanity = ReadUInt32();
    const int32_t rawVersion = ReadInt32();
    if (sanity != kMetadataSanity)
    {
        throw MetadataFormatError("invalid metadata sanity");
    }
    if (rawVersion < 0 || rawVersion > 1000)
    {
        throw MetadataFormatError("invalid metadata file");
    }
    if (rawVersion < kMetadataVersionMin || rawVersion > kMetadataVersionMax)
    {
        throw MetadataFormatError("unsupported metadata version");
    }

    version_ = static_cast<double>(rawVersion);
    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Metadata::Parse version={}", rawVersion));

    SetPosition(0);
    header_ = ReadGlobalMetadataHeader();
    DetectVersion24Variants();
    ReadTables();

    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] Metadata::Parse end images={} types={} methods={}",
            imageDefs_.size(),
            typeDefs_.size(),
            methodDefs_.size()));
}

void Metadata::DetectVersion24Variants()
{
    if (static_cast<int>(version_) != 24)
    {
        return;
    }

    if (header_.stringLiteralOffset == 264u)
    {
        version_ = 24.2;
        SetPosition(0);
        header_ = ReadGlobalMetadataHeader();
    }
    else
    {
        const std::vector<Il2CppImageDefinition> probeImages = ReadMetadataTable<Il2CppImageDefinition>(
            header_.imagesOffset,
            header_.imagesSize,
            ImageDefinitionSize());
        for (const Il2CppImageDefinition& imageDef : probeImages)
        {
            if (imageDef.token != 1u)
            {
                version_ = 24.1;
                break;
            }
        }
    }

    imageDefs_ = ReadMetadataTable<Il2CppImageDefinition>(
        header_.imagesOffset,
        header_.imagesSize,
        ImageDefinitionSize());

    if (version_ == 24.2)
    {
        const int32_t assemblyEntrySize = 68;
        if (header_.assembliesSize / assemblyEntrySize < static_cast<int32_t>(imageDefs_.size()))
        {
            version_ = 24.4;
            SetPosition(0);
            header_ = ReadGlobalMetadataHeader();
            imageDefs_ = ReadMetadataTable<Il2CppImageDefinition>(
                header_.imagesOffset,
                header_.imagesSize,
                ImageDefinitionSize());
        }
    }

    // Il2CppDumper also distinguishes a 24.1 header whose assembly records are
    // 64 bytes. This parser still does not consume the assemblies table, so that
    // probe belongs with a future assemblyDefs_ implementation.
    imageDefs_.clear();
}

} // namespace er2
