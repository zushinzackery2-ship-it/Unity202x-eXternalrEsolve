#include <er2/unity2/dumpsdk/offline/Metadata.h>

namespace er2
{

size_t Metadata::ImageDefinitionSize() const
{
    size_t size = sizeof(uint32_t) + sizeof(int32_t) + sizeof(int32_t) + sizeof(uint32_t);
    if (VersionInRange(version_, 24.0, 0.0, true, false))
    {
        size += sizeof(int32_t) + sizeof(uint32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    if (VersionInRange(version_, 24.1, 0.0, true, false))
    {
        size += sizeof(int32_t) + sizeof(uint32_t);
    }
    return size;
}

Il2CppImageDefinition Metadata::ReadImageDefinition()
{
    Il2CppImageDefinition value{};
    value.nameIndex = ReadUInt32();
    value.assemblyIndex = ReadInt32();
    value.typeStart = ReadInt32();
    value.typeCount = ReadUInt32();
    if (VersionInRange(version_, 24.0, 0.0, true, false))
    {
        value.exportedTypeStart = ReadInt32();
        value.exportedTypeCount = ReadUInt32();
    }
    value.entryPointIndex = ReadInt32();
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    if (VersionInRange(version_, 24.1, 0.0, true, false))
    {
        value.customAttributeStart = ReadInt32();
        value.customAttributeCount = ReadUInt32();
    }
    return value;
}

size_t Metadata::TypeDefinitionSize() const
{
    size_t size = sizeof(uint32_t) * 2;
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 24.5, false, true))
    {
        size += sizeof(int32_t);
    }
    size += sizeof(int32_t) * 3;
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        size += sizeof(int32_t) * 2;
    }
    size += sizeof(int32_t);
    if (VersionInRange(version_, 0.0, 22.0, false, true))
    {
        size += sizeof(int32_t) * 2;
    }
    if (VersionInRange(version_, 21.0, 22.0, true, true))
    {
        size += sizeof(int32_t) * 2;
    }
    size += sizeof(uint32_t);
    size += sizeof(int32_t) * 8;
    size += sizeof(uint16_t) * 8;
    size += sizeof(uint32_t);
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        size += sizeof(uint32_t);
    }
    return size;
}

Il2CppTypeDefinition Metadata::ReadTypeDefinition()
{
    Il2CppTypeDefinition value{};
    value.nameIndex = ReadUInt32();
    value.namespaceIndex = ReadUInt32();
    if (VersionInRange(version_, 0.0, 24.0, false, true))
    {
        value.customAttributeIndex = ReadInt32();
    }
    value.byvalTypeIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.5, false, true))
    {
        value.byrefTypeIndex = ReadInt32();
    }
    value.declaringTypeIndex = ReadInt32();
    value.parentIndex = ReadInt32();
    value.elementTypeIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 24.1, false, true))
    {
        value.rgctxStartIndex = ReadInt32();
        value.rgctxCount = ReadInt32();
    }
    value.genericContainerIndex = ReadInt32();
    if (VersionInRange(version_, 0.0, 22.0, false, true))
    {
        value.delegateWrapperFromManagedToNativeIndex = ReadInt32();
        value.marshalingFunctionsIndex = ReadInt32();
    }
    if (VersionInRange(version_, 21.0, 22.0, true, true))
    {
        value.ccwFunctionIndex = ReadInt32();
        value.guidIndex = ReadInt32();
    }
    value.flags = ReadUInt32();
    value.fieldStart = ReadInt32();
    value.methodStart = ReadInt32();
    value.eventStart = ReadInt32();
    value.propertyStart = ReadInt32();
    value.nestedTypesStart = ReadInt32();
    value.interfacesStart = ReadInt32();
    value.vtableStart = ReadInt32();
    value.interfaceOffsetsStart = ReadInt32();
    value.method_count = ReadUInt16();
    value.property_count = ReadUInt16();
    value.field_count = ReadUInt16();
    value.event_count = ReadUInt16();
    value.nested_type_count = ReadUInt16();
    value.vtable_count = ReadUInt16();
    value.interfaces_count = ReadUInt16();
    value.interface_offsets_count = ReadUInt16();
    value.bitfield = ReadUInt32();
    if (VersionInRange(version_, 19.0, 0.0, true, false))
    {
        value.token = ReadUInt32();
    }
    return value;
}

} // namespace er2
