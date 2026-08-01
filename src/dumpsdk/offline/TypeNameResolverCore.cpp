#include <er2/unity2/dumpsdk/offline/TypeNameResolver.h>

namespace er2
{

TypeNameResolver::TypeNameResolver(
    const Metadata& metadata,
    const OfflineRuntimeContext& context)
    : metadata_(metadata)
    , context_(context)
{
}

std::string TypeNameResolver::PrimitiveName(Il2CppTypeEnum type) const
{
    switch (type)
    {
    case Il2CppTypeEnum::IL2CPP_TYPE_VOID: return "void";
    case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN: return "bool";
    case Il2CppTypeEnum::IL2CPP_TYPE_CHAR: return "char";
    case Il2CppTypeEnum::IL2CPP_TYPE_I1: return "sbyte";
    case Il2CppTypeEnum::IL2CPP_TYPE_U1: return "byte";
    case Il2CppTypeEnum::IL2CPP_TYPE_I2: return "short";
    case Il2CppTypeEnum::IL2CPP_TYPE_U2: return "ushort";
    case Il2CppTypeEnum::IL2CPP_TYPE_I4: return "int";
    case Il2CppTypeEnum::IL2CPP_TYPE_U4: return "uint";
    case Il2CppTypeEnum::IL2CPP_TYPE_I8: return "long";
    case Il2CppTypeEnum::IL2CPP_TYPE_U8: return "ulong";
    case Il2CppTypeEnum::IL2CPP_TYPE_R4: return "float";
    case Il2CppTypeEnum::IL2CPP_TYPE_R8: return "double";
    case Il2CppTypeEnum::IL2CPP_TYPE_STRING: return "string";
    case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF: return "TypedReference";
    case Il2CppTypeEnum::IL2CPP_TYPE_I: return "IntPtr";
    case Il2CppTypeEnum::IL2CPP_TYPE_U: return "UIntPtr";
    case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT: return "object";
    case Il2CppTypeEnum::IL2CPP_TYPE_FNPTR: return "IntPtr";
    default: return "object";
    }
}

bool TypeNameResolver::TryGetTypeDefinition(
    const Il2CppTypeRuntime& type,
    Il2CppTypeDefinition& out) const
{
    const std::vector<Il2CppTypeDefinition>& typeDefs = metadata_.TypeDefs();
    if (context_.Version() >= 27.0 && context_.IsDumped())
    {
        const uint64_t handle = type.TypeHandle();
        const uint64_t base = context_.MetadataVirtualAddress() +
            metadata_.Header().typeDefinitionsOffset;
        if (handle < base)
        {
            return false;
        }
        const size_t recordSize = metadata_.TypeDefinitionRecordSize();
        const uint64_t delta = handle - base;
        if (recordSize == 0 || delta % recordSize != 0)
        {
            return false;
        }
        const uint64_t index = delta / recordSize;
        if (index >= typeDefs.size())
        {
            return false;
        }
        out = typeDefs[static_cast<size_t>(index)];
        return true;
    }

    const int64_t index = type.KlassIndex();
    if (index < 0 || static_cast<size_t>(index) >= typeDefs.size())
    {
        return false;
    }
    out = typeDefs[static_cast<size_t>(index)];
    return true;
}

bool TypeNameResolver::TryGetGenericParameter(
    const Il2CppTypeRuntime& type,
    Il2CppGenericParameter& out) const
{
    const std::vector<Il2CppGenericParameter>& parameters = metadata_.GenericParameters();
    if (context_.Version() >= 27.0 && context_.IsDumped())
    {
        const uint64_t handle = type.GenericParameterHandle();
        const uint64_t base = context_.MetadataVirtualAddress() +
            metadata_.Header().genericParametersOffset;
        if (handle < base)
        {
            return false;
        }
        const size_t recordSize = metadata_.GenericParameterRecordSize();
        const uint64_t delta = handle - base;
        if (recordSize == 0 || delta % recordSize != 0)
        {
            return false;
        }
        const uint64_t index = delta / recordSize;
        if (index >= parameters.size())
        {
            return false;
        }
        out = parameters[static_cast<size_t>(index)];
        return true;
    }

    const int64_t index = type.GenericParameterIndex();
    if (index < 0 || static_cast<size_t>(index) >= parameters.size())
    {
        return false;
    }
    out = parameters[static_cast<size_t>(index)];
    return true;
}

bool TypeNameResolver::TryGetGenericClassTypeDefinition(
    const Il2CppGenericClass& genericClass,
    Il2CppTypeDefinition& out) const
{
    if (context_.Version() >= 27.0)
    {
        Il2CppTypeRuntime type{};
        return genericClass.type != 0 &&
            context_.TryGetTypeByPointer(static_cast<uintptr_t>(genericClass.type), type) &&
            TryGetTypeDefinition(type, out);
    }
    const std::vector<Il2CppTypeDefinition>& typeDefs = metadata_.TypeDefs();
    if (genericClass.typeDefinitionIndex < 0 ||
        static_cast<size_t>(genericClass.typeDefinitionIndex) >= typeDefs.size())
    {
        return false;
    }
    out = typeDefs[static_cast<size_t>(genericClass.typeDefinitionIndex)];
    return true;
}

} // namespace er2
