#include <er2/unity2/dumpsdk/offline/Metadata.h>

namespace er2
{

Il2CppStringLiteral Metadata::ReadStringLiteral()
{
    Il2CppStringLiteral value{};
    value.length = ReadUInt32();
    value.dataIndex = ReadInt32();
    return value;
}

size_t Metadata::EventDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) * 4;
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    return size;
}

Il2CppEventDefinition Metadata::ReadEventDefinition()
{
    Il2CppEventDefinition value{};
    value.nameIndex = ReadUInt32();
    value.typeIndex = ReadInt32();
    value.add = ReadInt32();
    value.remove = ReadInt32();
    value.raise = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    return value;
}

Il2CppGenericContainer Metadata::ReadGenericContainer()
{
    Il2CppGenericContainer value{};
    value.ownerIndex = ReadInt32();
    value.type_argc = ReadInt32();
    value.is_method = ReadInt32();
    value.genericParameterStart = ReadInt32();
    return value;
}

size_t Metadata::GenericParameterSize() const
{
    return sizeof(int32_t) + sizeof(uint32_t) + sizeof(int16_t) * 2 + sizeof(uint16_t) * 2;
}

Il2CppGenericParameter Metadata::ReadGenericParameter()
{
    Il2CppGenericParameter value{};
    value.ownerIndex = ReadInt32();
    value.nameIndex = ReadUInt32();
    value.constraintsStart = ReadInt16();
    value.constraintsCount = ReadInt16();
    value.num = ReadUInt16();
    value.flags = ReadUInt16();
    return value;
}

Il2CppFieldDefaultValue Metadata::ReadFieldDefaultValue()
{
    Il2CppFieldDefaultValue value{};
    value.fieldIndex = ReadInt32();
    value.typeIndex = ReadInt32();
    value.dataIndex = ReadInt32();
    return value;
}

Il2CppParameterDefaultValue Metadata::ReadParameterDefaultValue()
{
    Il2CppParameterDefaultValue value{};
    value.parameterIndex = ReadInt32();
    value.typeIndex = ReadInt32();
    value.dataIndex = ReadInt32();
    return value;
}

size_t Metadata::CustomAttributeTypeRangeSize() const
{
    size_t size = sizeof(int32_t) * 2;
    if (VersionInRange(version_, 24.1, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    return size;
}

Il2CppCustomAttributeTypeRange Metadata::ReadCustomAttributeTypeRange()
{
    Il2CppCustomAttributeTypeRange value{};
    if (VersionInRange(version_, 24.1, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    value.start = ReadInt32();
    value.count = ReadInt32();
    return value;
}

Il2CppCustomAttributeDataRange Metadata::ReadCustomAttributeDataRange()
{
    Il2CppCustomAttributeDataRange value{};
    value.token = ReadUInt32();
    value.startOffset = ReadUInt32();
    return value;
}

std::string Metadata::GetStringFromIndex(uint32_t index) const
{
    const auto found = stringCache_.find(index);
    if (found != stringCache_.end())
    {
        return found->second;
    }

    Metadata reader = CreateBoundView();
    const std::string value = reader.ReadStringToNull(header_.stringOffset + index);
    stringCache_.emplace(index, value);
    return value;
}

std::string Metadata::GetStringLiteralFromIndex(uint32_t index) const
{
    if (index >= stringLiterals_.size())
    {
        throw StreamBoundsError("string literal index out of range");
    }

    const Il2CppStringLiteral& literal = stringLiterals_[index];
    Metadata reader = CreateBoundView();
    reader.SetPosition(static_cast<size_t>(header_.stringLiteralDataOffset) + static_cast<size_t>(literal.dataIndex));
    const std::vector<uint8_t> bytes = reader.ReadBytes(literal.length);
    return std::string(bytes.begin(), bytes.end());
}

} // namespace er2
