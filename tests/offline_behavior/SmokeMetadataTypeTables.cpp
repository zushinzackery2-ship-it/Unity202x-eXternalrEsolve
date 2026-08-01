#include "SmokeMetadataInternal.h"

namespace OfflineBehavior
{

namespace
{

struct TypeDefRecord
{
    uint32_t nameIndex = 0;
    uint32_t namespaceIndex = 0;
    int32_t byvalTypeIndex = -1;
    int32_t declaringTypeIndex = -1;
    int32_t parentIndex = -1;
    int32_t elementTypeIndex = -1;
    int32_t genericContainerIndex = -1;
    uint32_t flags = 0;
    int32_t fieldStart = -1;
    int32_t methodStart = -1;
    int32_t eventStart = -1;
    int32_t propertyStart = -1;
    int32_t nestedTypesStart = -1;
    int32_t interfacesStart = -1;
    int32_t vtableStart = 0;
    int32_t interfaceOffsetsStart = -1;
    uint16_t methodCount = 0;
    uint16_t propertyCount = 0;
    uint16_t fieldCount = 0;
    uint16_t eventCount = 0;
    uint16_t nestedTypeCount = 0;
    uint16_t vtableCount = 0;
    uint16_t interfacesCount = 0;
    uint16_t interfaceOffsetsCount = 0;
    uint32_t bitfield = 0;
    uint32_t token = 0;
};

struct MethodRecord
{
    uint32_t nameIndex = 0;
    int32_t declaringType = 0;
    int32_t returnType = 0;
    int32_t parameterStart = -1;
    int32_t genericContainerIndex = -1;
    uint32_t token = 0;
    uint16_t flags = 0;
    uint16_t iflags = 0;
    uint16_t slot = 0xFFFF;
    uint16_t parameterCount = 0;
};

constexpr uint32_t kTypePublic = 0x00000001;
constexpr uint32_t kTypeNestedFamOrAssem = 0x00000007;
constexpr uint32_t kTypeInterface = 0x00000020;
constexpr uint32_t kTypeAbstract = 0x00000080;
constexpr uint32_t kTypeSealed = 0x00000100;
constexpr uint32_t kTypeSerializable = 0x00002000;

constexpr uint16_t kMethodPublic = 0x0006;
constexpr uint16_t kMethodStatic = 0x0010;
constexpr uint16_t kMethodFinal = 0x0020;
constexpr uint16_t kMethodVirtual = 0x0040;
constexpr uint16_t kMethodNewSlot = 0x0100;
constexpr uint16_t kMethodAbstract = 0x0400;
constexpr uint16_t kMethodSpecialName = 0x0800;

void WriteTypeDef(Writer& writer, const TypeDefRecord& type)
{
    writer.U32(type.nameIndex);
    writer.U32(type.namespaceIndex);
    writer.I32(type.byvalTypeIndex);
    writer.I32(type.declaringTypeIndex);
    writer.I32(type.parentIndex);
    writer.I32(type.elementTypeIndex);
    writer.I32(type.genericContainerIndex);
    writer.U32(type.flags);
    writer.I32(type.fieldStart);
    writer.I32(type.methodStart);
    writer.I32(type.eventStart);
    writer.I32(type.propertyStart);
    writer.I32(type.nestedTypesStart);
    writer.I32(type.interfacesStart);
    writer.I32(type.vtableStart);
    writer.I32(type.interfaceOffsetsStart);
    writer.U16(type.methodCount);
    writer.U16(type.propertyCount);
    writer.U16(type.fieldCount);
    writer.U16(type.eventCount);
    writer.U16(type.nestedTypeCount);
    writer.U16(type.vtableCount);
    writer.U16(type.interfacesCount);
    writer.U16(type.interfaceOffsetsCount);
    writer.U32(type.bitfield);
    writer.U32(type.token);
}

void WriteMethod(Writer& writer, const MethodRecord& method)
{
    writer.U32(method.nameIndex);
    writer.I32(method.declaringType);
    writer.I32(method.returnType);
    writer.I32(method.parameterStart);
    writer.I32(method.genericContainerIndex);
    writer.U32(method.token);
    writer.U16(method.flags);
    writer.U16(method.iflags);
    writer.U16(method.slot);
    writer.U16(method.parameterCount);
}

std::vector<TypeDefRecord> BuildTypeDefs(StringTable& strings)
{
    std::vector<TypeDefRecord> types(kTypeCount);
    auto name = [&](SmokeTypeIndex index, const char* nameSpace, const char* simpleName)
    {
        types[index].namespaceIndex = strings.Add(nameSpace);
        types[index].nameIndex = strings.Add(simpleName);
        types[index].byvalTypeIndex = index;
    };

    name(kTypeObject, "System", "Object");
    types[kTypeObject].flags = kTypePublic;
    name(kTypeValueType, "System", "ValueType");
    types[kTypeValueType].flags = kTypePublic | kTypeAbstract;
    types[kTypeValueType].parentIndex = kRtObject;
    name(kTypeEnum, "System", "Enum");
    types[kTypeEnum].flags = kTypePublic | kTypeAbstract;
    types[kTypeEnum].parentIndex = kRtValueType;
    name(kTypeInt32, "System", "Int32");
    types[kTypeInt32].flags = kTypePublic | kTypeSealed | kTypeSerializable;
    types[kTypeInt32].parentIndex = kRtValueType;
    types[kTypeInt32].bitfield = 0x1;
    name(kTypeString, "System", "String");
    types[kTypeString].flags = kTypePublic | kTypeSealed;
    types[kTypeString].parentIndex = kRtObject;
    name(kTypeIDisposable, "Smoke", "IDisposable");
    types[kTypeIDisposable].flags = kTypePublic | kTypeInterface | kTypeAbstract;

    name(kTypeBase, "Smoke", "BaseType");
    types[kTypeBase].flags = kTypePublic | kTypeAbstract;
    types[kTypeBase].parentIndex = kRtObject;
    types[kTypeBase].methodStart = kMethodBaseRun;
    types[kTypeBase].methodCount = 1;
    types[kTypeBase].vtableCount = 1;

    name(kTypeDerived, "Smoke", "Derived");
    types[kTypeDerived].flags = kTypePublic | kTypeSealed | kTypeSerializable;
    types[kTypeDerived].parentIndex = kRtBase;
    types[kTypeDerived].fieldStart = 0;
    types[kTypeDerived].fieldCount = 6;
    types[kTypeDerived].methodStart = kMethodGetCounter;
    types[kTypeDerived].methodCount = 8;
    types[kTypeDerived].propertyStart = 0;
    types[kTypeDerived].propertyCount = 1;
    types[kTypeDerived].eventStart = 0;
    types[kTypeDerived].eventCount = 1;
    types[kTypeDerived].interfacesStart = 0;
    types[kTypeDerived].interfacesCount = 1;
    types[kTypeDerived].nestedTypesStart = 0;
    types[kTypeDerived].nestedTypeCount = 1;
    types[kTypeDerived].vtableStart = 1;
    types[kTypeDerived].vtableCount = 1;

    name(kTypeNested, "", "Nested");
    types[kTypeNested].flags = kTypeNestedFamOrAssem;
    types[kTypeNested].parentIndex = kRtObject;
    types[kTypeNested].declaringTypeIndex = kRtDerived;
    types[kTypeNested].fieldStart = 6;
    types[kTypeNested].fieldCount = 1;

    name(kTypeGeneric, "Smoke", "Container`1");
    types[kTypeGeneric].flags = kTypePublic;
    types[kTypeGeneric].parentIndex = kRtObject;
    types[kTypeGeneric].genericContainerIndex = 0;
    types[kTypeGeneric].methodStart = kMethodContainerGet;
    types[kTypeGeneric].methodCount = 1;

    name(kTypeAttribute, "Smoke", "MarkerAttribute");
    types[kTypeAttribute].flags = kTypePublic | kTypeSealed;
    types[kTypeAttribute].parentIndex = kRtObject;
    types[kTypeAttribute].methodStart = kMethodAttributeCtor;
    types[kTypeAttribute].methodCount = 1;

    for (size_t i = 0; i < types.size(); ++i)
    {
        types[i].token = kTypeTokenBase + static_cast<uint32_t>(i);
    }
    return types;
}

std::vector<MethodRecord> BuildMethods(StringTable& strings)
{
    std::vector<MethodRecord> methods(kMethodCount);
    auto define = [&](SmokeMethodIndex index, const char* name, int32_t declaringType, int32_t returnType)
    {
        methods[index].nameIndex = strings.Add(name);
        methods[index].declaringType = declaringType;
        methods[index].returnType = returnType;
        methods[index].flags = kMethodPublic;
    };

    define(kMethodBaseRun, "Run", kTypeBase, kRtVoid);
    methods[kMethodBaseRun].flags |= kMethodVirtual | kMethodNewSlot | kMethodAbstract;
    methods[kMethodBaseRun].slot = 0;
    define(kMethodGetCounter, "get_Counter", kTypeDerived, kRtInt32);
    methods[kMethodGetCounter].flags |= kMethodSpecialName;
    define(kMethodSetCounter, "set_Counter", kTypeDerived, kRtVoid);
    methods[kMethodSetCounter].flags |= kMethodSpecialName;
    methods[kMethodSetCounter].parameterStart = 0;
    methods[kMethodSetCounter].parameterCount = 1;
    define(kMethodAddChanged, "add_Changed", kTypeDerived, kRtVoid);
    methods[kMethodAddChanged].flags |= kMethodSpecialName;
    methods[kMethodAddChanged].parameterStart = 1;
    methods[kMethodAddChanged].parameterCount = 1;
    define(kMethodRemoveChanged, "remove_Changed", kTypeDerived, kRtVoid);
    methods[kMethodRemoveChanged].flags |= kMethodSpecialName;
    methods[kMethodRemoveChanged].parameterStart = 2;
    methods[kMethodRemoveChanged].parameterCount = 1;
    define(kMethodDerivedRun, "Run", kTypeDerived, kRtVoid);
    methods[kMethodDerivedRun].flags |= kMethodFinal | kMethodVirtual;
    methods[kMethodDerivedRun].slot = 0;
    define(kMethodCompute, "Compute", kTypeDerived, kRtInt32);
    methods[kMethodCompute].flags |= kMethodStatic;
    methods[kMethodCompute].parameterStart = 3;
    methods[kMethodCompute].parameterCount = 5;
    define(kMethodDerivedCtor, ".ctor", kTypeDerived, kRtVoid);
    methods[kMethodDerivedCtor].flags |= kMethodSpecialName;
    define(kMethodEcho, "Echo", kTypeDerived, kRtMVar0);
    methods[kMethodEcho].genericContainerIndex = 1;
    methods[kMethodEcho].parameterStart = 8;
    methods[kMethodEcho].parameterCount = 1;
    define(kMethodContainerGet, "Get", kTypeGeneric, kRtVar0);
    define(kMethodAttributeCtor, ".ctor", kTypeAttribute, kRtVoid);
    methods[kMethodAttributeCtor].flags |= kMethodSpecialName;
    methods[kMethodAttributeCtor].parameterStart = 9;
    methods[kMethodAttributeCtor].parameterCount = 1;

    for (size_t i = 0; i < methods.size(); ++i)
    {
        methods[i].token = kMethodTokenBase + static_cast<uint32_t>(i);
    }
    return methods;
}

} // namespace

void BuildTypeAndMethodTables(StringTable& strings, SmokeTables& tables)
{
    Writer typeWriter;
    for (const TypeDefRecord& type : BuildTypeDefs(strings))
    {
        WriteTypeDef(typeWriter, type);
    }
    tables.typeDefs = std::move(typeWriter.bytes);

    Writer methodWriter;
    for (const MethodRecord& method : BuildMethods(strings))
    {
        WriteMethod(methodWriter, method);
    }
    tables.methods = std::move(methodWriter.bytes);
}

} // namespace OfflineBehavior
