#pragma once

#include <er2/unity2/dumpsdk/offline/MetadataFlags.h>

#include <cstdint>

namespace er2
{

struct Il2CppImageDefinition
{
    uint32_t nameIndex = 0;
    int32_t assemblyIndex = 0;
    int32_t typeStart = 0;
    uint32_t typeCount = 0;
    int32_t exportedTypeStart = 0;
    uint32_t exportedTypeCount = 0;
    int32_t entryPointIndex = 0;
    uint32_t token = 0;
    int32_t customAttributeStart = 0;
    uint32_t customAttributeCount = 0;
};

struct Il2CppTypeDefinition
{
    uint32_t nameIndex = 0;
    uint32_t namespaceIndex = 0;
    int32_t customAttributeIndex = 0;
    int32_t byvalTypeIndex = 0;
    int32_t byrefTypeIndex = 0;
    int32_t declaringTypeIndex = 0;
    int32_t parentIndex = 0;
    int32_t elementTypeIndex = 0;
    int32_t rgctxStartIndex = 0;
    int32_t rgctxCount = 0;
    int32_t genericContainerIndex = 0;
    int32_t delegateWrapperFromManagedToNativeIndex = 0;
    int32_t marshalingFunctionsIndex = 0;
    int32_t ccwFunctionIndex = 0;
    int32_t guidIndex = 0;
    uint32_t flags = 0;
    int32_t fieldStart = 0;
    int32_t methodStart = 0;
    int32_t eventStart = 0;
    int32_t propertyStart = 0;
    int32_t nestedTypesStart = 0;
    int32_t interfacesStart = 0;
    int32_t vtableStart = 0;
    int32_t interfaceOffsetsStart = 0;
    uint16_t method_count = 0;
    uint16_t property_count = 0;
    uint16_t field_count = 0;
    uint16_t event_count = 0;
    uint16_t nested_type_count = 0;
    uint16_t vtable_count = 0;
    uint16_t interfaces_count = 0;
    uint16_t interface_offsets_count = 0;
    uint32_t bitfield = 0;
    uint32_t token = 0;

    bool IsValueType() const
    {
        return (bitfield & kIl2CppTypeValueType) != 0;
    }

    bool IsEnum() const
    {
        return (bitfield & kIl2CppTypeEnumType) != 0;
    }
};

struct Il2CppMethodDefinition
{
    uint32_t nameIndex = 0;
    int32_t declaringType = 0;
    int32_t returnType = 0;
    int32_t returnParameterToken = 0;
    int32_t parameterStart = 0;
    int32_t customAttributeIndex = 0;
    int32_t genericContainerIndex = 0;
    int32_t methodIndex = 0;
    int32_t invokerIndex = 0;
    int32_t delegateWrapperIndex = 0;
    int32_t rgctxStartIndex = 0;
    int32_t rgctxCount = 0;
    uint32_t token = 0;
    uint16_t flags = 0;
    uint16_t iflags = 0;
    uint16_t slot = kNoVTableSlot;
    uint16_t parameterCount = 0;
};

struct Il2CppParameterDefinition
{
    uint32_t nameIndex = 0;
    uint32_t token = 0;
    int32_t customAttributeIndex = 0;
    int32_t typeIndex = 0;
};

struct Il2CppFieldDefinition
{
    uint32_t nameIndex = 0;
    int32_t typeIndex = 0;
    int32_t customAttributeIndex = 0;
    uint32_t token = 0;
};

struct Il2CppPropertyDefinition
{
    uint32_t nameIndex = 0;
    int32_t get = 0;
    int32_t set = 0;
    uint32_t attrs = 0;
    int32_t customAttributeIndex = 0;
    uint32_t token = 0;
};

struct Il2CppEventDefinition
{
    uint32_t nameIndex = 0;
    int32_t typeIndex = 0;
    int32_t add = 0;
    int32_t remove = 0;
    int32_t raise = 0;
    int32_t customAttributeIndex = 0;
    uint32_t token = 0;
};

struct Il2CppStringLiteral
{
    uint32_t length = 0;
    int32_t dataIndex = 0;
};

struct Il2CppGenericContainer
{
    int32_t ownerIndex = 0;
    int32_t type_argc = 0;
    int32_t is_method = 0;
    int32_t genericParameterStart = 0;
};

struct Il2CppGenericParameter
{
    int32_t ownerIndex = 0;
    uint32_t nameIndex = 0;
    int16_t constraintsStart = 0;
    int16_t constraintsCount = 0;
    uint16_t num = 0;
    uint16_t flags = 0;
};

struct Il2CppFieldRef
{
    int32_t typeIndex = 0;
    int32_t fieldIndex = 0;
};

struct Il2CppFieldDefaultValue
{
    int32_t fieldIndex = 0;
    int32_t typeIndex = 0;
    int32_t dataIndex = 0;
};

struct Il2CppParameterDefaultValue
{
    int32_t parameterIndex = 0;
    int32_t typeIndex = 0;
    int32_t dataIndex = 0;
};

struct Il2CppCustomAttributeTypeRange
{
    uint32_t token = 0;
    int32_t start = 0;
    int32_t count = 0;
};

struct Il2CppCustomAttributeDataRange
{
    uint32_t token = 0;
    uint32_t startOffset = 0;
};

struct Il2CppMetadataUsageList
{
    uint32_t start = 0;
    uint32_t count = 0;
};

struct Il2CppMetadataUsagePair
{
    uint32_t destinationIndex = 0;
    uint32_t encodedSourceIndex = 0;
};

} // namespace er2
