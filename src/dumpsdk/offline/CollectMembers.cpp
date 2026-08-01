#include "OfflineCollectorInternal.h"

namespace er2
{

void CollectTypeHierarchy(
    const CollectContext& context,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out)
{
    if (typeDef.parentIndex >= 0)
    {
        const Il2CppTypeRuntime* parent = context.runtime.GetTypeByIndex(typeDef.parentIndex);
        if (parent != nullptr)
        {
            const std::string parentName = context.resolver.GetTypeName(*parent, false, false);
            if (!typeDef.IsValueType() && !typeDef.IsEnum() && parentName != "object")
            {
                out.parentName = parentName;
            }
        }
    }

    const std::vector<int32_t>& interfaces = context.metadata.InterfaceIndices();
    for (uint16_t i = 0; i < typeDef.interfaces_count; ++i)
    {
        const int64_t index = static_cast<int64_t>(typeDef.interfacesStart) + i;
        if (index < 0 || static_cast<size_t>(index) >= interfaces.size())
        {
            continue;
        }
        const Il2CppTypeRuntime* interfaceType =
            context.runtime.GetTypeByIndex(interfaces[static_cast<size_t>(index)]);
        if (interfaceType != nullptr)
        {
            out.interfaces.push_back(context.resolver.GetTypeName(*interfaceType, false, false));
        }
    }
}

void CollectFields(
    const CollectContext& context,
    size_t imageIndex,
    size_t typeDefIndex,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out)
{
    const std::vector<Il2CppFieldDefinition>& fields = context.metadata.FieldDefs();
    for (uint16_t i = 0; i < typeDef.field_count; ++i)
    {
        const int64_t flatIndex = static_cast<int64_t>(typeDef.fieldStart) + i;
        if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= fields.size())
        {
            continue;
        }
        const Il2CppFieldDefinition& fieldDef = fields[static_cast<size_t>(flatIndex)];
        const Il2CppTypeRuntime* fieldType = context.runtime.GetTypeByIndex(fieldDef.typeIndex);

        CollectedField field{};
        field.name = context.metadata.GetStringFromIndex(fieldDef.nameIndex);
        field.typeName = fieldType == nullptr
            ? "object"
            : context.resolver.GetTypeName(*fieldType, false, false);
        field.token = fieldDef.token;
        field.flags = fieldType == nullptr ? 0u : fieldType->attrs;
        field.isLiteral = (field.flags & kFieldLiteral) != 0;
        field.isStatic = !field.isLiteral && (field.flags & kFieldStatic) != 0;
        field.isReadOnly = !field.isLiteral && (field.flags & kFieldInitOnly) != 0;
        field.accessModifier = AccessFromFieldFlags(field.flags);
        field.attributes = context.attributes.Render(
            imageIndex,
            fieldDef.customAttributeIndex,
            fieldDef.token);

        Il2CppFieldDefaultValue defaultValue{};
        if (context.metadata.TryGetFieldDefaultValue(
                static_cast<int32_t>(flatIndex),
                defaultValue) &&
            defaultValue.dataIndex != -1)
        {
            field.defaultValueIsComment = !context.decoder.TryRenderDefaultValue(
                defaultValue.typeIndex,
                defaultValue.dataIndex,
                field.defaultValue);
        }

        if (!field.isLiteral)
        {
            const int32_t offset = context.runtime.GetFieldOffset(
                static_cast<int32_t>(typeDefIndex),
                static_cast<int32_t>(i),
                static_cast<int32_t>(flatIndex),
                typeDef.IsValueType(),
                field.isStatic);
            field.hasOffset = offset >= 0;
            field.offset = offset >= 0 ? static_cast<size_t>(offset) : 0;
        }
        out.fields.push_back(std::move(field));
    }
}

void CollectProperties(
    const CollectContext& context,
    size_t imageIndex,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out)
{
    const std::vector<Il2CppPropertyDefinition>& properties = context.metadata.PropertyDefs();
    const std::vector<Il2CppMethodDefinition>& methods = context.metadata.MethodDefs();
    const std::vector<Il2CppParameterDefinition>& parameters = context.metadata.ParameterDefs();

    auto methodAt = [&](int32_t localIndex) -> const Il2CppMethodDefinition*
    {
        const int64_t index = static_cast<int64_t>(typeDef.methodStart) + localIndex;
        return index < 0 || static_cast<size_t>(index) >= methods.size()
            ? nullptr
            : &methods[static_cast<size_t>(index)];
    };

    for (uint16_t i = 0; i < typeDef.property_count; ++i)
    {
        const int64_t flatIndex = static_cast<int64_t>(typeDef.propertyStart) + i;
        if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= properties.size())
        {
            continue;
        }
        const Il2CppPropertyDefinition& definition = properties[static_cast<size_t>(flatIndex)];
        CollectedProperty property{};
        property.name = context.metadata.GetStringFromIndex(definition.nameIndex);
        property.token = definition.token;
        property.hasGetter = definition.get >= 0;
        property.hasSetter = definition.set >= 0;
        property.attributes = context.attributes.Render(
            imageIndex,
            definition.customAttributeIndex,
            definition.token);

        const Il2CppMethodDefinition* accessor = methodAt(
            definition.get >= 0 ? definition.get : definition.set);
        if (accessor != nullptr)
        {
            property.modifiers = AccessFromMethodFlags(accessor->flags);
            if (!property.modifiers.empty())
            {
                property.modifiers += " ";
            }
            property.modifiers += MethodModifiersFromFlags(accessor->flags);
        }

        if (definition.get >= 0)
        {
            const Il2CppMethodDefinition* getter = methodAt(definition.get);
            const Il2CppTypeRuntime* type = getter == nullptr
                ? nullptr
                : context.runtime.GetTypeByIndex(getter->returnType);
            property.typeName = type == nullptr
                ? "object"
                : context.resolver.GetTypeName(*type, false, false);
        }
        else if (accessor != nullptr && accessor->parameterStart >= 0 &&
            static_cast<size_t>(accessor->parameterStart) < parameters.size())
        {
            const Il2CppTypeRuntime* type = context.runtime.GetTypeByIndex(
                parameters[static_cast<size_t>(accessor->parameterStart)].typeIndex);
            property.typeName = type == nullptr
                ? "object"
                : context.resolver.GetTypeName(*type, false, false);
        }
        if (property.typeName.empty())
        {
            property.typeName = "object";
        }
        out.properties.push_back(std::move(property));
    }
}

void CollectEvents(
    const CollectContext& context,
    size_t imageIndex,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out)
{
    const std::vector<Il2CppEventDefinition>& events = context.metadata.EventDefs();
    const std::vector<Il2CppMethodDefinition>& methods = context.metadata.MethodDefs();
    for (uint16_t i = 0; i < typeDef.event_count; ++i)
    {
        const int64_t flatIndex = static_cast<int64_t>(typeDef.eventStart) + i;
        if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= events.size())
        {
            continue;
        }
        const Il2CppEventDefinition& definition = events[static_cast<size_t>(flatIndex)];
        CollectedEvent event{};
        event.name = context.metadata.GetStringFromIndex(definition.nameIndex);
        event.token = definition.token;
        event.hasAdd = definition.add >= 0;
        event.hasRemove = definition.remove >= 0;
        event.hasRaise = definition.raise >= 0;
        event.attributes = context.attributes.Render(
            imageIndex,
            definition.customAttributeIndex,
            definition.token);
        event.typeName = context.resolver.GetTypeNameByIndex(definition.typeIndex, false);

        const int32_t localAccessor = definition.add >= 0 ? definition.add : definition.remove;
        const int64_t accessorIndex = static_cast<int64_t>(typeDef.methodStart) + localAccessor;
        if (localAccessor >= 0 && accessorIndex >= 0 &&
            static_cast<size_t>(accessorIndex) < methods.size())
        {
            const uint32_t flags = methods[static_cast<size_t>(accessorIndex)].flags;
            event.modifiers = AccessFromMethodFlags(flags);
            if (!event.modifiers.empty())
            {
                event.modifiers += " ";
            }
            event.modifiers += MethodModifiersFromFlags(flags);
        }
        out.events.push_back(std::move(event));
    }
}

} // namespace er2
