#pragma once

#include <cstdint>

namespace er2
{

constexpr int kMetadataVersionMin = 16;
constexpr int kMetadataVersionMax = 31;

enum class Il2CppMetadataUsage : uint32_t
{
    Invalid = 0,
    TypeInfo = 1,
    Il2CppType = 2,
    MethodDef = 3,
    FieldInfo = 4,
    StringLiteral = 5,
    MethodRef = 6,
    kIl2CppMetadataUsageInvalid = Invalid,
    kIl2CppMetadataUsageTypeInfo = TypeInfo,
    kIl2CppMetadataUsageIl2CppType = Il2CppType,
    kIl2CppMetadataUsageMethodDef = MethodDef,
    kIl2CppMetadataUsageFieldInfo = FieldInfo,
    kIl2CppMetadataUsageStringLiteral = StringLiteral,
    kIl2CppMetadataUsageMethodRef = MethodRef,
};

enum Il2CppTypeDefinitionBitfield : uint32_t
{
    kIl2CppTypeValueType = 0x1,
    kIl2CppTypeEnumType = 0x2,
};

enum Il2CppFieldFlags : uint32_t
{
    kFieldAccessMask = 0x0007,
    kFieldCompilerControlled = 0x0000,
    kFieldPrivate = 0x0001,
    kFieldFamAndAssem = 0x0002,
    kFieldAssembly = 0x0003,
    kFieldFamily = 0x0004,
    kFieldFamOrAssem = 0x0005,
    kFieldPublic = 0x0006,
    kFieldStatic = 0x0010,
    kFieldInitOnly = 0x0020,
    kFieldLiteral = 0x0040,
};

enum Il2CppMethodFlags : uint32_t
{
    kMethodMemberAccessMask = 0x0007,
    kMethodCompilerControlled = 0x0000,
    kMethodPrivate = 0x0001,
    kMethodFamAndAssem = 0x0002,
    kMethodAssem = 0x0003,
    kMethodFamily = 0x0004,
    kMethodFamOrAssem = 0x0005,
    kMethodPublic = 0x0006,
    kMethodStatic = 0x0010,
    kMethodFinal = 0x0020,
    kMethodVirtual = 0x0040,
    kMethodVTableLayoutMask = 0x0100,
    kMethodReuseSlot = 0x0000,
    kMethodNewSlot = 0x0100,
    kMethodAbstract = 0x0400,
    kMethodPInvokeImpl = 0x2000,
};

enum Il2CppTypeFlags : uint32_t
{
    kTypeVisibilityMask = 0x0007,
    kTypeNotPublic = 0x0000,
    kTypePublic = 0x0001,
    kTypeNestedPublic = 0x0002,
    kTypeNestedPrivate = 0x0003,
    kTypeNestedFamily = 0x0004,
    kTypeNestedAssembly = 0x0005,
    kTypeNestedFamAndAssem = 0x0006,
    kTypeNestedFamOrAssem = 0x0007,
    kTypeInterface = 0x0020,
    kTypeAbstract = 0x0080,
    kTypeSealed = 0x0100,
    kTypeSerializable = 0x2000,
};

enum Il2CppParamFlags : uint32_t
{
    kParamIn = 0x0001,
    kParamOut = 0x0002,
    kParamOptional = 0x0010,
};

constexpr uint16_t kNoVTableSlot = 0xFFFFu;

} // namespace er2
