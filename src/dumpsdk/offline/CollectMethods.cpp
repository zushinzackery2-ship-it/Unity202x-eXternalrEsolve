#include "OfflineCollectorInternal.h"

namespace er2
{

namespace
{

void CollectParams(
    const CollectContext& context,
    const Il2CppMethodDefinition& methodDef,
    CollectedMethod& out)
{
    const std::vector<Il2CppParameterDefinition>& parameters = context.metadata.ParameterDefs();
    for (uint16_t i = 0; i < methodDef.parameterCount; ++i)
    {
        const int64_t flatIndex = static_cast<int64_t>(methodDef.parameterStart) + i;
        if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= parameters.size())
        {
            continue;
        }
        const Il2CppParameterDefinition& definition = parameters[static_cast<size_t>(flatIndex)];
        const Il2CppTypeRuntime* type = context.runtime.GetTypeByIndex(definition.typeIndex);
        CollectedParam parameter{};
        parameter.name = context.metadata.GetStringFromIndex(definition.nameIndex);
        parameter.typeName = type == nullptr
            ? "object"
            : context.resolver.GetTypeName(*type, false, false);

        if (type != nullptr)
        {
            const bool hasIn = (type->attrs & kParamIn) != 0;
            const bool hasOut = (type->attrs & kParamOut) != 0;
            if (type->byref == 1)
            {
                if (hasOut && !hasIn)
                {
                    parameter.isOut = true;
                }
                else if (hasIn && !hasOut)
                {
                    parameter.isIn = true;
                }
                else
                {
                    parameter.isByRef = true;
                }
            }
            else
            {
                if (hasIn)
                {
                    parameter.attributePrefix += "[In] ";
                }
                if (hasOut)
                {
                    parameter.attributePrefix += "[Out] ";
                }
            }
        }

        Il2CppParameterDefaultValue defaultValue{};
        if (context.metadata.TryGetParameterDefaultValue(
                static_cast<int32_t>(flatIndex),
                defaultValue) &&
            defaultValue.dataIndex != -1)
        {
            parameter.defaultValueIsComment = !context.decoder.TryRenderDefaultValue(
                defaultValue.typeIndex,
                defaultValue.dataIndex,
                parameter.defaultValue);
        }
        out.params.push_back(std::move(parameter));
    }
}

void CollectGenericInstGroups(
    const CollectContext& context,
    int32_t methodDefinitionIndex,
    CollectedMethod& out)
{
    const std::vector<OfflineRuntimeContext::MethodSpecEntry>* entries =
        context.runtime.GetMethodSpecsForMethod(methodDefinitionIndex);
    if (entries == nullptr)
    {
        return;
    }
    for (const OfflineRuntimeContext::MethodSpecEntry& entry : *entries)
    {
        std::string typeName;
        std::string methodName;
        if (!context.resolver.GetMethodSpecName(entry.spec, typeName, methodName))
        {
            continue;
        }

        CollectedGenericInstGroup* group = nullptr;
        for (CollectedGenericInstGroup& candidate : out.genericInstGroups)
        {
            if (candidate.address == entry.genericMethodPointer)
            {
                group = &candidate;
                break;
            }
        }
        if (group == nullptr)
        {
            CollectedGenericInstGroup created{};
            created.address = entry.genericMethodPointer;
            if (entry.genericMethodPointer > context.runtime.Pe().ImageBase())
            {
                created.rva = static_cast<uint32_t>(
                    entry.genericMethodPointer - context.runtime.Pe().ImageBase());
                created.fileOffset = static_cast<uint32_t>(
                    context.runtime.Pe().MapFileOffset(entry.genericMethodPointer));
            }
            out.genericInstGroups.push_back(std::move(created));
            group = &out.genericInstGroups.back();
        }
        group->entries.push_back(typeName + "." + methodName);
    }
}

} // namespace

void CollectMethods(
    const CollectContext& context,
    size_t imageIndex,
    const std::string& imageName,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out)
{
    const std::vector<Il2CppMethodDefinition>& methods = context.metadata.MethodDefs();
    const std::vector<Il2CppGenericContainer>& containers = context.metadata.GenericContainers();
    for (uint16_t i = 0; i < typeDef.method_count; ++i)
    {
        const int64_t flatIndex = static_cast<int64_t>(typeDef.methodStart) + i;
        if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= methods.size())
        {
            continue;
        }
        const Il2CppMethodDefinition& definition = methods[static_cast<size_t>(flatIndex)];
        CollectedMethod method{};
        method.name = context.metadata.GetStringFromIndex(definition.nameIndex);
        method.token = definition.token;
        method.flags = definition.flags;
        method.iflags = definition.iflags;
        method.slot = definition.slot;
        method.isStatic = (definition.flags & kMethodStatic) != 0;
        method.isInstance = !method.isStatic;
        method.isAbstract = (definition.flags & kMethodAbstract) != 0;
        method.isVirtual = (definition.flags & kMethodVirtual) != 0;
        method.isSealed = (definition.flags & kMethodFinal) != 0;
        method.isExtern = (definition.flags & kMethodPInvokeImpl) != 0;
        method.isGeneric = definition.genericContainerIndex >= 0;
        const bool reuseSlot =
            (definition.flags & kMethodVTableLayoutMask) == kMethodReuseSlot;
        method.isOverride = reuseSlot &&
            ((definition.flags & (kMethodVirtual | kMethodAbstract | kMethodFinal)) != 0);
        method.accessModifier = AccessFromMethodFlags(definition.flags);
        method.attributes = context.attributes.Render(
            imageIndex,
            definition.customAttributeIndex,
            definition.token);

        if (method.isGeneric &&
            static_cast<size_t>(definition.genericContainerIndex) < containers.size())
        {
            method.name += context.resolver.GetGenericContainerParams(
                containers[static_cast<size_t>(definition.genericContainerIndex)]);
        }

        const Il2CppTypeRuntime* returnType = context.runtime.GetTypeByIndex(definition.returnType);
        method.returnType = returnType == nullptr
            ? "object"
            : context.resolver.GetTypeName(*returnType, false, false);
        method.returnIsByRef = returnType != nullptr && returnType->byref == 1;

        const uintptr_t pointer = method.isAbstract
            ? 0
            : context.runtime.GetMethodPointer(imageName, definition);
        if (pointer > context.runtime.Pe().ImageBase())
        {
            method.address = pointer;
            method.rva = static_cast<uint32_t>(pointer - context.runtime.Pe().ImageBase());
            method.fileOffset = static_cast<uint32_t>(context.runtime.Pe().MapFileOffset(pointer));
        }

        CollectParams(context, definition, method);
        CollectGenericInstGroups(context, static_cast<int32_t>(flatIndex), method);
        out.methods.push_back(std::move(method));
    }
}

} // namespace er2
