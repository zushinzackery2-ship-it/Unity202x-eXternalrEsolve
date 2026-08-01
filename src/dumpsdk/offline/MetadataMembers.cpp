#include <er2/unity2/dumpsdk/offline/Metadata.h>

namespace er2
{

size_t Metadata::MethodDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) * 2;
    if (VersionInRange(version_, 31.0, 0.0, true, false))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        size += sizeof(int32_t) * 5;
    }
    size += sizeof(uint32_t) + sizeof(uint16_t) * 4;
    return size;
}

Il2CppMethodDefinition Metadata::ReadMethodDefinition()
{
    Il2CppMethodDefinition value{};
    value.nameIndex = ReadUInt32();
    value.declaringType = ReadInt32();
    value.returnType = ReadInt32();
    if (VersionInRange(version_, 31.0, 0.0, true, false))
    {
        value.returnParameterToken = ReadInt32();
    }
    value.parameterStart = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    value.genericContainerIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        value.methodIndex = ReadInt32();
        value.invokerIndex = ReadInt32();
        value.delegateWrapperIndex = ReadInt32();
        value.rgctxStartIndex = ReadInt32();
        value.rgctxCount = ReadInt32();
    }
    value.token = ReadUInt32();
    value.flags = ReadUInt16();
    value.iflags = ReadUInt16();
    value.slot = ReadUInt16();
    value.parameterCount = ReadUInt16();
    return value;
}

size_t Metadata::ParameterDefinitionSize() const
{
    size_t size = sizeof(uint32_t) * 2;
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    return size;
}

Il2CppParameterDefinition Metadata::ReadParameterDefinition()
{
    Il2CppParameterDefinition value{};
    value.nameIndex = ReadUInt32();
    value.token = ReadUInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    value.typeIndex = ReadInt32();
    return value;
}

size_t Metadata::FieldDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t);
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

Il2CppFieldDefinition Metadata::ReadFieldDefinition()
{
    Il2CppFieldDefinition value{};
    value.nameIndex = ReadUInt32();
    value.typeIndex = ReadInt32();
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

size_t Metadata::PropertyDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) * 2 + sizeof(uint32_t);
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

Il2CppPropertyDefinition Metadata::ReadPropertyDefinition()
{
    Il2CppPropertyDefinition value{};
    value.nameIndex = ReadUInt32();
    value.get = ReadInt32();
    value.set = ReadInt32();
    value.attrs = ReadUInt32();
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

} // namespace er2
