#include <er2/unity2/dumpsdk/offline/CustomAttributeReader.h>

namespace er2
{

namespace
{

std::string StripAttributeSuffix(const std::string& name)
{
    constexpr const char* kSuffix = "Attribute";
    constexpr size_t kSuffixLength = 9;
    if (name.size() > kSuffixLength &&
        name.compare(name.size() - kSuffixLength, kSuffixLength, kSuffix) == 0)
    {
        return name.substr(0, name.size() - kSuffixLength);
    }
    return name;
}

} // namespace

std::vector<std::string> CustomAttributeReader::RenderPost29(int32_t attributeIndex) const
{
    const std::vector<Il2CppCustomAttributeDataRange>& ranges = metadata_.AttributeDataRanges();
    if (static_cast<size_t>(attributeIndex) + 1 >= ranges.size())
    {
        return {};
    }
    const uint32_t startOffset = ranges[static_cast<size_t>(attributeIndex)].startOffset;
    const uint32_t endOffset = ranges[static_cast<size_t>(attributeIndex) + 1].startOffset;
    if (endOffset <= startOffset)
    {
        return {};
    }

    const size_t blobStart = static_cast<size_t>(metadata_.Header().attributeDataOffset) + startOffset;
    const size_t blobEnd = static_cast<size_t>(metadata_.Header().attributeDataOffset) + endOffset;
    if (blobEnd > metadata_.Size())
    {
        return {};
    }

    BinaryStream header(metadata_.Data(), metadata_.Size());
    uint32_t count = 0;
    size_t ctorCursor = 0;
    try
    {
        header.SetPosition(blobStart);
        count = header.ReadCompressedUInt32();
        ctorCursor = header.Position();
    }
    catch (const StreamBoundsError&)
    {
        return {};
    }
    if (count == 0 || count > 0x10000u)
    {
        return {};
    }

    size_t dataCursor = ctorCursor + static_cast<size_t>(count) * sizeof(int32_t);
    const std::vector<Il2CppMethodDefinition>& methodDefs = metadata_.MethodDefs();
    const std::vector<Il2CppTypeDefinition>& typeDefs = metadata_.TypeDefs();
    const std::vector<Il2CppFieldDefinition>& fieldDefs = metadata_.FieldDefs();
    const std::vector<Il2CppPropertyDefinition>& propertyDefs = metadata_.PropertyDefs();

    std::vector<std::string> result;
    for (uint32_t i = 0; i < count; ++i)
    {
        int32_t ctorIndex = 0;
        try
        {
            BinaryStream reader(metadata_.Data(), metadata_.Size());
            reader.SetPosition(ctorCursor);
            ctorIndex = reader.ReadInt32();
            ctorCursor = reader.Position();
        }
        catch (const StreamBoundsError&)
        {
            break;
        }
        if (ctorIndex < 0 || static_cast<size_t>(ctorIndex) >= methodDefs.size())
        {
            break;
        }
        const Il2CppMethodDefinition& ctor = methodDefs[static_cast<size_t>(ctorIndex)];
        if (ctor.declaringType < 0 || static_cast<size_t>(ctor.declaringType) >= typeDefs.size())
        {
            break;
        }
        const Il2CppTypeDefinition& attributeType =
            typeDefs[static_cast<size_t>(ctor.declaringType)];

        uint32_t argumentCount = 0;
        uint32_t fieldCount = 0;
        uint32_t propertyCount = 0;
        try
        {
            BinaryStream reader(metadata_.Data(), metadata_.Size());
            reader.SetPosition(dataCursor);
            argumentCount = reader.ReadCompressedUInt32();
            fieldCount = reader.ReadCompressedUInt32();
            propertyCount = reader.ReadCompressedUInt32();
            dataCursor = reader.Position();
        }
        catch (const StreamBoundsError&)
        {
            break;
        }

        auto readMemberIndex = [&](const Il2CppTypeDefinition*& declaring, int32_t& memberIndex)
        {
            try
            {
                BinaryStream reader(metadata_.Data(), metadata_.Size());
                reader.SetPosition(dataCursor);
                memberIndex = reader.ReadCompressedInt32();
                if (memberIndex >= 0)
                {
                    declaring = &attributeType;
                    dataCursor = reader.Position();
                    return true;
                }
                memberIndex = -(memberIndex + 1);
                const uint32_t typeIndex = reader.ReadCompressedUInt32();
                dataCursor = reader.Position();
                if (typeIndex >= typeDefs.size())
                {
                    return false;
                }
                declaring = &typeDefs[typeIndex];
                return true;
            }
            catch (const StreamBoundsError&)
            {
                return false;
            }
        };

        std::vector<std::string> arguments;
        bool failed = false;
        for (uint32_t argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex)
        {
            std::string value;
            if (!decoder_.TryRenderAttributeValue(dataCursor, value))
            {
                failed = true;
                break;
            }
            arguments.push_back(value);
        }
        for (uint32_t fieldIndex = 0; fieldIndex < fieldCount && !failed; ++fieldIndex)
        {
            std::string value;
            const Il2CppTypeDefinition* declaring = nullptr;
            int32_t memberIndex = 0;
            if (!decoder_.TryRenderAttributeValue(dataCursor, value) ||
                !readMemberIndex(declaring, memberIndex) || declaring == nullptr)
            {
                failed = true;
                break;
            }
            const int64_t flatIndex = static_cast<int64_t>(declaring->fieldStart) + memberIndex;
            if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= fieldDefs.size())
            {
                failed = true;
                break;
            }
            arguments.push_back(metadata_.GetStringFromIndex(
                fieldDefs[static_cast<size_t>(flatIndex)].nameIndex) + " = " + value);
        }
        for (uint32_t propertyIndex = 0; propertyIndex < propertyCount && !failed; ++propertyIndex)
        {
            std::string value;
            const Il2CppTypeDefinition* declaring = nullptr;
            int32_t memberIndex = 0;
            if (!decoder_.TryRenderAttributeValue(dataCursor, value) ||
                !readMemberIndex(declaring, memberIndex) || declaring == nullptr)
            {
                failed = true;
                break;
            }
            const int64_t flatIndex = static_cast<int64_t>(declaring->propertyStart) + memberIndex;
            if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= propertyDefs.size())
            {
                failed = true;
                break;
            }
            arguments.push_back(metadata_.GetStringFromIndex(
                propertyDefs[static_cast<size_t>(flatIndex)].nameIndex) + " = " + value);
        }

        const std::string typeName = StripAttributeSuffix(
            metadata_.GetStringFromIndex(attributeType.nameIndex));
        std::string rendered = "[" + typeName;
        if (!arguments.empty())
        {
            rendered += "(";
            for (size_t argumentIndex = 0; argumentIndex < arguments.size(); ++argumentIndex)
            {
                if (argumentIndex > 0)
                {
                    rendered += ", ";
                }
                rendered += arguments[argumentIndex];
            }
            rendered += ")";
        }
        result.push_back(rendered + "]");
        if (failed)
        {
            break;
        }
    }
    return result;
}

} // namespace er2
