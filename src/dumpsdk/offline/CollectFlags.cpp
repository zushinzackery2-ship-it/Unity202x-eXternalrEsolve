#include "OfflineCollectorInternal.h"

namespace er2
{

std::string AccessFromTypeFlags(uint32_t flags)
{
    switch (flags & kTypeVisibilityMask)
    {
    case kTypePublic:
    case kTypeNestedPublic:
        return "public";
    case kTypeNotPublic:
    case kTypeNestedFamAndAssem:
    case kTypeNestedAssembly:
        return "internal";
    case kTypeNestedPrivate:
        return "private";
    case kTypeNestedFamily:
        return "protected";
    case kTypeNestedFamOrAssem:
        return "protected internal";
    default:
        return "internal";
    }
}

std::string AccessFromFieldFlags(uint32_t flags)
{
    switch (flags & kFieldAccessMask)
    {
    case kFieldPrivate: return "private";
    case kFieldPublic: return "public";
    case kFieldFamily: return "protected";
    case kFieldAssembly:
    case kFieldFamAndAssem:
        return "internal";
    case kFieldFamOrAssem:
        return "protected internal";
    default:
        return {};
    }
}

std::string AccessFromMethodFlags(uint32_t flags)
{
    switch (flags & kMethodMemberAccessMask)
    {
    case kMethodPrivate: return "private";
    case kMethodPublic: return "public";
    case kMethodFamily: return "protected";
    case kMethodAssem:
    case kMethodFamAndAssem:
        return "internal";
    case kMethodFamOrAssem:
        return "protected internal";
    default:
        return {};
    }
}

std::string MethodModifiersFromFlags(uint32_t flags)
{
    std::string result;
    if ((flags & kMethodStatic) != 0)
    {
        result += "static ";
    }
    if ((flags & kMethodAbstract) != 0)
    {
        result += "abstract ";
        if ((flags & kMethodVTableLayoutMask) == kMethodReuseSlot)
        {
            result += "override ";
        }
    }
    else if ((flags & kMethodFinal) != 0)
    {
        if ((flags & kMethodVTableLayoutMask) == kMethodReuseSlot)
        {
            result += "sealed override ";
        }
    }
    else if ((flags & kMethodVirtual) != 0)
    {
        result += (flags & kMethodVTableLayoutMask) == kMethodNewSlot
            ? "virtual "
            : "override ";
    }
    if ((flags & kMethodPInvokeImpl) != 0)
    {
        result += "extern ";
    }
    return result;
}

TypeKind ResolveTypeKind(const Il2CppTypeDefinition& typeDef)
{
    if ((typeDef.flags & kTypeInterface) != 0)
    {
        return TypeKind::Interface;
    }
    if (typeDef.IsEnum())
    {
        return TypeKind::Enum;
    }
    if (typeDef.IsValueType())
    {
        return TypeKind::Struct;
    }
    return TypeKind::Class;
}

} // namespace er2
