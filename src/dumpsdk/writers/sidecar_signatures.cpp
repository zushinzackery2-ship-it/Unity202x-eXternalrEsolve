#include "sidecar_writer_internal.hpp"

namespace er2::SidecarDetail
{

std::string BuildTypeDeclaration(const CollectedType& type)
{
    std::string result;
    if (!type.accessModifier.empty())
    {
        result = type.accessModifier + " ";
    }
    const bool interfaceType = type.kind == TypeKind::Interface;
    const bool valueLike = type.kind == TypeKind::Struct || type.kind == TypeKind::Enum;
    if (type.isAbstract && type.isSealed)
    {
        result += "static ";
    }
    else if (!interfaceType && type.isAbstract)
    {
        result += "abstract ";
    }
    else if (!valueLike && type.isSealed)
    {
        result += "sealed ";
    }
    switch (type.kind)
    {
    case TypeKind::Interface: result += "interface "; break;
    case TypeKind::Enum: result += "enum "; break;
    case TypeKind::Struct: result += "struct "; break;
    case TypeKind::Delegate: result += "class "; break;
    default: result += "class "; break;
    }
    return result + type.name;
}

std::string BuildBaseClause(const CollectedType& type)
{
    std::string bases = type.parentName;
    for (const std::string& interfaceName : type.interfaces)
    {
        if (!bases.empty())
        {
            bases += ", ";
        }
        bases += interfaceName;
    }
    return bases.empty() ? std::string{} : " : " + bases;
}

std::string BuildFieldModifiers(const CollectedField& field)
{
    std::string result;
    if (!field.accessModifier.empty())
    {
        result = field.accessModifier + " ";
    }
    if (field.isLiteral)
    {
        return result + "const ";
    }
    if (field.isStatic)
    {
        result += "static ";
    }
    if (field.isReadOnly)
    {
        result += "readonly ";
    }
    return result;
}

std::string BuildMethodModifiers(const CollectedMethod& method)
{
    std::string result;
    if (!method.accessModifier.empty())
    {
        result = method.accessModifier + " ";
    }
    if (method.isStatic)
    {
        result += "static ";
    }
    if (method.isAbstract)
    {
        result += "abstract ";
        if (method.isOverride)
        {
            result += "override ";
        }
    }
    else if (method.isSealed && method.isOverride)
    {
        result += "sealed override ";
    }
    else if (method.isVirtual)
    {
        result += method.isOverride ? "override " : "virtual ";
    }
    if (method.isExtern)
    {
        result += "extern ";
    }
    return result;
}

std::string BuildParamText(const CollectedParam& parameter)
{
    std::string result = parameter.attributePrefix;
    if (parameter.isOut)
    {
        result += "out ";
    }
    else if (parameter.isIn)
    {
        result += "in ";
    }
    else if (parameter.isByRef)
    {
        result += "ref ";
    }
    result += parameter.typeName + " " + parameter.name;
    if (!parameter.defaultValue.empty())
    {
        result += parameter.defaultValueIsComment
            ? parameter.defaultValue
            : " = " + parameter.defaultValue;
    }
    return result;
}

std::string BuildMethodSignature(
    const CollectedMethod& method,
    const std::string& className,
    bool isStatic)
{
    const std::string safeClass = EscapeCppName(className);
    std::string signature = ToCppType(method.returnType, false) + " " +
        safeClass + "$$" + method.name + " (";
    bool wroteParameter = false;
    if (!isStatic)
    {
        signature += safeClass + "_o* __this";
        wroteParameter = true;
    }
    for (const CollectedParam& parameter : method.params)
    {
        if (wroteParameter)
        {
            signature += ", ";
        }
        signature += ToCppType(parameter.typeName, false);
        if (parameter.isOut || parameter.isIn || parameter.isByRef)
        {
            signature += "*";
        }
        signature += " " + (parameter.name.empty() ? std::string("arg") : parameter.name);
        wroteParameter = true;
    }
    if (wroteParameter)
    {
        signature += ", ";
    }
    return signature + "const MethodInfo* method);";
}

std::string BuildTypeSignature(const CollectedMethod& method)
{
    auto classify = [](const std::string& typeName)
    {
        if (typeName == "long" || typeName == "ulong" ||
            typeName == "Int64" || typeName == "UInt64")
        {
            return 'j';
        }
        if (typeName == "float" || typeName == "Single")
        {
            return 'f';
        }
        if (typeName == "double" || typeName == "Double")
        {
            return 'd';
        }
        return 'i';
    };
    std::string signature;
    signature += method.returnType == "void" ? 'v' : classify(method.returnType);
    if (!method.isStatic)
    {
        signature += 'i';
    }
    for (const CollectedParam& parameter : method.params)
    {
        signature += classify(parameter.typeName);
    }
    return signature + 'i';
}

} // namespace er2::SidecarDetail
