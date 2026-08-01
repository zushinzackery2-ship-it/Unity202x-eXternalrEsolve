#include <er2/unity2/dumpsdk/offline/TypeNameResolver.h>

namespace er2
{

std::string TypeNameResolver::GetTypeName(
    const Il2CppTypeRuntime& type,
    bool addNamespace,
    bool isNested) const
{
    switch (type.type)
    {
    case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY:
    {
        Il2CppArrayType arrayType{};
        Il2CppTypeRuntime elementType{};
        if (!ReadIl2CppArrayType(context_.Pe(), type.ArrayType(), arrayType) ||
            !context_.TryGetTypeByPointer(static_cast<uintptr_t>(arrayType.etype), elementType))
        {
            return "object[]";
        }
        const int commas = arrayType.rank > 1 ? arrayType.rank - 1 : 0;
        return GetTypeName(elementType, addNamespace, false) + "[" +
            std::string(static_cast<size_t>(commas), ',') + "]";
    }
    case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
    {
        Il2CppTypeRuntime elementType{};
        if (!context_.TryGetTypeByPointer(static_cast<uintptr_t>(type.NestedType()), elementType))
        {
            return "object[]";
        }
        return GetTypeName(elementType, addNamespace, false) + "[]";
    }
    case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
    {
        Il2CppTypeRuntime pointee{};
        if (!context_.TryGetTypeByPointer(static_cast<uintptr_t>(type.NestedType()), pointee))
        {
            return "void*";
        }
        return GetTypeName(pointee, addNamespace, false) + "*";
    }
    case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
    case Il2CppTypeEnum::IL2CPP_TYPE_MVAR:
    {
        Il2CppGenericParameter parameter{};
        if (!TryGetGenericParameter(type, parameter))
        {
            return "T";
        }
        const std::string name = metadata_.GetStringFromIndex(parameter.nameIndex);
        return name.empty() ? "T" : name;
    }
    case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
    case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
    case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST:
    {
        Il2CppTypeDefinition typeDef{};
        Il2CppGenericClass genericClass{};
        bool hasGenericClass = false;
        if (type.type == Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST)
        {
            if (!ReadIl2CppGenericClass(
                    context_.Pe(),
                    static_cast<uintptr_t>(type.GenericClass()),
                    context_.Version(),
                    genericClass) ||
                !TryGetGenericClassTypeDefinition(genericClass, typeDef))
            {
                return "object";
            }
            hasGenericClass = true;
        }
        else if (!TryGetTypeDefinition(type, typeDef))
        {
            return "object";
        }

        std::string result;
        if (typeDef.declaringTypeIndex != -1)
        {
            const Il2CppTypeRuntime* declaring = context_.GetTypeByIndex(typeDef.declaringTypeIndex);
            if (declaring != nullptr)
            {
                result = GetTypeName(*declaring, addNamespace, true) + ".";
            }
        }
        else if (addNamespace)
        {
            const std::string nameSpace = metadata_.GetStringFromIndex(typeDef.namespaceIndex);
            if (!nameSpace.empty())
            {
                result = nameSpace + ".";
            }
        }

        const std::string rawName = metadata_.GetStringFromIndex(typeDef.nameIndex);
        const size_t tick = rawName.find('`');
        result += tick == std::string::npos ? rawName : rawName.substr(0, tick);
        if (isNested)
        {
            return result;
        }
        if (hasGenericClass && genericClass.context.class_inst != 0)
        {
            Il2CppGenericInst inst{};
            if (ReadIl2CppGenericInst(
                    context_.Pe(),
                    static_cast<uintptr_t>(genericClass.context.class_inst),
                    inst))
            {
                result += GetGenericInstParams(inst);
            }
        }
        else if (typeDef.genericContainerIndex >= 0 &&
            static_cast<size_t>(typeDef.genericContainerIndex) < metadata_.GenericContainers().size())
        {
            result += GetGenericContainerParams(
                metadata_.GenericContainers()[static_cast<size_t>(typeDef.genericContainerIndex)]);
        }
        return result;
    }
    default:
        return PrimitiveName(type.type);
    }
}

std::string TypeNameResolver::GetTypeNameByIndex(int32_t typeIndex, bool addNamespace) const
{
    const Il2CppTypeRuntime* type = context_.GetTypeByIndex(typeIndex);
    return type == nullptr ? "object" : GetTypeName(*type, addNamespace, false);
}

std::string TypeNameResolver::GetTypeDefName(
    const Il2CppTypeDefinition& typeDef,
    bool addNamespace,
    bool genericParameter) const
{
    std::string prefix;
    if (typeDef.declaringTypeIndex != -1)
    {
        const Il2CppTypeRuntime* declaring = context_.GetTypeByIndex(typeDef.declaringTypeIndex);
        if (declaring != nullptr)
        {
            prefix = GetTypeName(*declaring, addNamespace, true) + ".";
        }
    }
    else if (addNamespace)
    {
        const std::string nameSpace = metadata_.GetStringFromIndex(typeDef.namespaceIndex);
        if (!nameSpace.empty())
        {
            prefix = nameSpace + ".";
        }
    }

    std::string typeName = metadata_.GetStringFromIndex(typeDef.nameIndex);
    if (typeDef.genericContainerIndex >= 0)
    {
        const size_t tick = typeName.find('`');
        if (tick != std::string::npos)
        {
            typeName = typeName.substr(0, tick);
        }
        if (genericParameter &&
            static_cast<size_t>(typeDef.genericContainerIndex) < metadata_.GenericContainers().size())
        {
            typeName += GetGenericContainerParams(
                metadata_.GenericContainers()[static_cast<size_t>(typeDef.genericContainerIndex)]);
        }
    }
    return prefix + typeName;
}

} // namespace er2
