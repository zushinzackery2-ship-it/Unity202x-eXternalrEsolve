#include <er2/unity2/dumpsdk/offline/TypeNameResolver.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

namespace er2
{

std::string TypeNameResolver::GetGenericContainerParams(
    const Il2CppGenericContainer& container) const
{
    const std::vector<Il2CppGenericParameter>& parameters = metadata_.GenericParameters();
    std::string result = "<";
    for (int32_t i = 0; i < container.type_argc; ++i)
    {
        if (i > 0)
        {
            result += ", ";
        }
        const int64_t index = static_cast<int64_t>(container.genericParameterStart) + i;
        if (index < 0 || static_cast<size_t>(index) >= parameters.size())
        {
            result += "T";
            continue;
        }
        const std::string name = metadata_.GetStringFromIndex(
            parameters[static_cast<size_t>(index)].nameIndex);
        result += name.empty() ? "T" : name;
    }
    return result + ">";
}

std::string TypeNameResolver::GetGenericInstParams(const Il2CppGenericInst& inst) const
{
    std::string result = "<";
    for (int64_t i = 0; i < inst.type_argc; ++i)
    {
        if (i > 0)
        {
            result += ", ";
        }
        uint64_t argumentPointer = 0;
        Il2CppTypeRuntime argumentType{};
        if (!TryReadU64(
                context_.Pe(),
                inst.type_argv + static_cast<uint64_t>(i) * sizeof(uintptr_t),
                argumentPointer) ||
            argumentPointer == 0 ||
            !context_.TryGetTypeByPointer(static_cast<uintptr_t>(argumentPointer), argumentType))
        {
            result += "object";
            continue;
        }
        result += GetTypeName(argumentType, false, false);
    }
    return result + ">";
}

bool TypeNameResolver::GetMethodSpecName(
    const Il2CppMethodSpec& spec,
    std::string& typeName,
    std::string& methodName) const
{
    const std::vector<Il2CppMethodDefinition>& methodDefs = metadata_.MethodDefs();
    if (spec.methodDefinitionIndex < 0 ||
        static_cast<size_t>(spec.methodDefinitionIndex) >= methodDefs.size())
    {
        return false;
    }
    const Il2CppMethodDefinition& methodDef =
        methodDefs[static_cast<size_t>(spec.methodDefinitionIndex)];
    const std::vector<Il2CppTypeDefinition>& typeDefs = metadata_.TypeDefs();
    if (methodDef.declaringType < 0 ||
        static_cast<size_t>(methodDef.declaringType) >= typeDefs.size())
    {
        return false;
    }

    typeName = GetTypeDefName(
        typeDefs[static_cast<size_t>(methodDef.declaringType)],
        false,
        false);
    const std::vector<Il2CppGenericInst>& insts = context_.GenericInsts();
    if (spec.classIndexIndex != -1 && static_cast<size_t>(spec.classIndexIndex) < insts.size())
    {
        typeName += GetGenericInstParams(insts[static_cast<size_t>(spec.classIndexIndex)]);
    }

    methodName = metadata_.GetStringFromIndex(methodDef.nameIndex);
    if (spec.methodIndexIndex != -1 && static_cast<size_t>(spec.methodIndexIndex) < insts.size())
    {
        methodName += GetGenericInstParams(insts[static_cast<size_t>(spec.methodIndexIndex)]);
    }
    return true;
}

} // namespace er2
