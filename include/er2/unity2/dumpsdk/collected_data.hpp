#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace er2
{

enum class TypeKind
{
    Class,
    Struct,
    Interface,
    Enum,
    Delegate,
};

struct CollectedParam
{
    std::string name;
    std::string typeName;
    bool isOut = false;
    bool isIn = false;
    bool isByRef = false;
    std::string attributePrefix;
    std::string defaultValue;
    bool defaultValueIsComment = false;
};

struct CollectedField
{
    std::string name;
    std::string typeName;
    size_t offset = 0;
    uint32_t token = 0;
    uint32_t flags = 0;
    bool isStatic = false;
    bool isLiteral = false;
    bool isReadOnly = false;
    bool hasOffset = false;
    std::string defaultValue;
    bool defaultValueIsComment = false;
    std::string accessModifier;
    std::vector<std::string> attributes;
};

struct CollectedProperty
{
    std::string name;
    std::string typeName;
    uint32_t token = 0;
    bool hasGetter = false;
    bool hasSetter = false;
    std::string modifiers;
    std::vector<std::string> attributes;
};

struct CollectedEvent
{
    std::string name;
    std::string typeName;
    uint32_t token = 0;
    bool hasAdd = false;
    bool hasRemove = false;
    bool hasRaise = false;
    std::string modifiers;
    std::vector<std::string> attributes;
};

struct CollectedGenericInstGroup
{
    uintptr_t address = 0;
    uint32_t rva = 0;
    uint32_t fileOffset = 0;
    std::vector<std::string> entries;
};

struct CollectedMethod
{
    std::string name;
    std::string returnType;
    std::vector<CollectedParam> params;
    uintptr_t address = 0;
    uint32_t rva = 0;
    uint32_t fileOffset = 0;
    uint32_t token = 0;
    uint32_t flags = 0;
    uint32_t iflags = 0;
    uint16_t slot = 0xFFFFu;
    bool isStatic = false;
    bool isInstance = false;
    bool isGeneric = false;
    bool isInflated = false;
    bool isAbstract = false;
    bool isVirtual = false;
    bool isOverride = false;
    bool isSealed = false;
    bool isExtern = false;
    bool returnIsByRef = false;
    std::string accessModifier;
    std::vector<std::string> attributes;
    std::vector<CollectedGenericInstGroup> genericInstGroups;
};

struct CollectedType
{
    std::string name;
    std::string namespaceName;
    std::string assemblyName;
    std::string parentName;
    std::vector<std::string> interfaces;
    TypeKind kind = TypeKind::Class;
    uint32_t token = 0;
    uint32_t flags = 0;
    size_t index = 0;
    size_t typeDefIndex = 0;
    size_t instanceSize = 0;
    bool isAbstract = false;
    bool isSealed = false;
    bool isGeneric = false;
    bool isPublic = false;
    bool isNested = false;
    bool isSerializable = false;
    bool isStaticClass = false;
    std::string accessModifier;
    std::vector<std::string> attributes;
    std::vector<CollectedField> fields;
    std::vector<CollectedProperty> properties;
    std::vector<CollectedEvent> events;
    std::vector<CollectedMethod> methods;
};

struct CollectedAssembly
{
    std::string name;
    std::string fileName;
    size_t imageIndex = 0;
    size_t typeStartIndex = 0;
    std::vector<CollectedType> types;
};

struct CollectedStringLiteral
{
    std::string value;
    uintptr_t address = 0;
};

struct CollectedMetadata
{
    std::string name;
    uintptr_t address = 0;
    std::string signature;
};

struct CollectedMethodRef
{
    std::string name;
    uintptr_t address = 0;
    uintptr_t methodAddress = 0;
};

struct CollectedData
{
    std::vector<CollectedAssembly> assemblies;
    std::vector<CollectedStringLiteral> strings;
    std::vector<CollectedMetadata> metadata;
    std::vector<CollectedMethodRef> methodRefs;
    double metadataVersion = 0.0;
    uintptr_t imageBase = 0;
    bool fromOffline = false;
};

} // namespace er2
