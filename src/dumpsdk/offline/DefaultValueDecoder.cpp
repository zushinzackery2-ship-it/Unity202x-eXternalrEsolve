#include <er2/unity2/dumpsdk/offline/DefaultValueDecoder.h>

#include <cstdio>
#include <format>

namespace er2
{

std::string EscapeCSharpString(const std::string& value)
{
    std::string result;
    result.reserve(value.size() + 8);
    for (const char c : value)
    {
        switch (c)
        {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        case '\0': result += "\\0"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                char buffer[8] = {};
                std::snprintf(buffer, sizeof(buffer), "\\x%02x", static_cast<unsigned char>(c));
                result += buffer;
            }
            else
            {
                result += c;
            }
            break;
        }
    }
    return result;
}

DefaultValueDecoder::DefaultValueDecoder(
    const Metadata& metadata,
    const OfflineRuntimeContext& context,
    const TypeNameResolver& resolver)
    : metadata_(metadata)
    , context_(context)
    , resolver_(resolver)
{
}

Il2CppTypeEnum DefaultValueDecoder::ReadEncodedTypeEnum(
    BinaryStream& reader,
    const Il2CppTypeRuntime** enumType) const
{
    if (enumType != nullptr)
    {
        *enumType = nullptr;
    }
    Il2CppTypeEnum type = static_cast<Il2CppTypeEnum>(reader.ReadUInt8());
    if (type != Il2CppTypeEnum::IL2CPP_TYPE_ENUM)
    {
        return type;
    }

    const int32_t enumTypeIndex = reader.ReadCompressedInt32();
    const Il2CppTypeRuntime* resolved = context_.GetTypeByIndex(enumTypeIndex);
    if (resolved == nullptr)
    {
        return Il2CppTypeEnum::IL2CPP_TYPE_I4;
    }
    if (enumType != nullptr)
    {
        *enumType = resolved;
    }
    Il2CppTypeDefinition typeDef{};
    if (!resolver_.TryGetTypeDefinition(*resolved, typeDef))
    {
        return Il2CppTypeEnum::IL2CPP_TYPE_I4;
    }
    const Il2CppTypeRuntime* elementType = context_.GetTypeByIndex(typeDef.elementTypeIndex);
    return elementType == nullptr ? Il2CppTypeEnum::IL2CPP_TYPE_I4 : elementType->type;
}

bool DefaultValueDecoder::RenderConstant(
    Il2CppTypeEnum type,
    BinaryStream& reader,
    std::string& text) const
{
    const bool compressed = context_.Version() >= 29.0;
    switch (type)
    {
    case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
        text = reader.ReadUInt8() != 0 ? "true" : "false";
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_U1:
        text = std::to_string(static_cast<uint32_t>(reader.ReadUInt8()));
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_I1:
        text = std::to_string(static_cast<int32_t>(reader.ReadInt8()));
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_CHAR:
        text = std::format("'\\x{:x}'", reader.ReadUInt16());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_U2:
        text = std::to_string(reader.ReadUInt16());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_I2:
        text = std::to_string(reader.ReadInt16());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_U4:
        text = std::to_string(compressed ? reader.ReadCompressedUInt32() : reader.ReadUInt32());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_I4:
        text = std::to_string(compressed ? reader.ReadCompressedInt32() : reader.ReadInt32());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_U8:
        text = std::to_string(reader.ReadUInt64());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_I8:
        text = std::to_string(reader.ReadInt64());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_R4:
        text = std::format("{}", reader.ReadSingle());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_R8:
        text = std::format("{}", reader.ReadDouble());
        return true;
    case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
    {
        const int32_t length = compressed ? reader.ReadCompressedInt32() : reader.ReadInt32();
        if (length < 0)
        {
            text = "null";
            return true;
        }
        const std::vector<uint8_t> bytes = reader.ReadBytes(static_cast<size_t>(length));
        text = "\"" + EscapeCSharpString(std::string(bytes.begin(), bytes.end())) + "\"";
        return true;
    }
    case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
    {
        const int32_t count = reader.ReadCompressedInt32();
        if (count < 0)
        {
            text = "null";
            return true;
        }
        const Il2CppTypeRuntime* enumType = nullptr;
        const Il2CppTypeEnum commonType = ReadEncodedTypeEnum(reader, &enumType);
        const bool heterogeneous = reader.ReadUInt8() == 1;
        std::string body;
        for (int32_t i = 0; i < count; ++i)
        {
            if (i > 0)
            {
                body += ", ";
            }
            const Il2CppTypeEnum elementType = heterogeneous
                ? ReadEncodedTypeEnum(reader, &enumType)
                : commonType;
            std::string element;
            if (!RenderConstant(elementType, reader, element))
            {
                return false;
            }
            body += element;
        }
        text = "new[] { " + body + " }";
        return true;
    }
    case Il2CppTypeEnum::IL2CPP_TYPE_INDEX:
    {
        const int32_t typeIndex = reader.ReadCompressedInt32();
        const Il2CppTypeRuntime* resolved = context_.GetTypeByIndex(typeIndex);
        if (typeIndex < 0 || resolved == nullptr)
        {
            text = "null";
            return true;
        }
        text = "typeof(" + resolver_.GetTypeName(*resolved, false, false) + ")";
        return true;
    }
    default:
        return false;
    }
}

bool DefaultValueDecoder::TryRenderDefaultValue(
    int32_t typeIndex,
    int32_t dataIndex,
    std::string& text) const
{
    text.clear();
    const uint32_t offset = metadata_.GetDefaultValueDataOffset(dataIndex);
    const Il2CppTypeRuntime* type = context_.GetTypeByIndex(typeIndex);
    if (type == nullptr)
    {
        text = std::format(" /*Metadata offset 0x{:X}*/", offset);
        return false;
    }
    BinaryStream reader(metadata_.Data(), metadata_.Size());
    try
    {
        reader.SetPosition(offset);
        if (RenderConstant(type->type, reader, text))
        {
            return true;
        }
    }
    catch (const StreamBoundsError&)
    {
        text.clear();
    }
    text = std::format(" /*Metadata offset 0x{:X}*/", offset);
    return false;
}

bool DefaultValueDecoder::TryRenderAttributeValue(size_t& position, std::string& text) const
{
    BinaryStream reader(metadata_.Data(), metadata_.Size());
    try
    {
        reader.SetPosition(position);
        const Il2CppTypeRuntime* enumType = nullptr;
        const bool ok = RenderConstant(ReadEncodedTypeEnum(reader, &enumType), reader, text);
        position = reader.Position();
        return ok;
    }
    catch (const StreamBoundsError&)
    {
        return false;
    }
}

} // namespace er2
