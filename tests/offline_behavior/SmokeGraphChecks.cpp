#include "SmokeAssertions.h"

#include <iostream>

namespace OfflineBehavior
{

namespace
{

const er2::CollectedType* FindType(const er2::CollectedData& data, const std::string& name)
{
    for (const er2::CollectedAssembly& assembly : data.assemblies)
    {
        for (const er2::CollectedType& type : assembly.types)
        {
            if (type.name == name)
            {
                return &type;
            }
        }
    }
    return nullptr;
}

const er2::CollectedField* FindField(const er2::CollectedType& type, const std::string& name)
{
    for (const er2::CollectedField& field : type.fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

const er2::CollectedMethod* FindMethod(const er2::CollectedType& type, const std::string& name)
{
    for (const er2::CollectedMethod& method : type.methods)
    {
        if (method.name == name)
        {
            return &method;
        }
    }
    return nullptr;
}

} // namespace

void CheckCollectedGraph(Checks& checks, const er2::CollectedData& data, const SmokeModule& module)
{
    std::cout << "-- Collected graph\n";
    checks.Equal(data.assemblies.size(), 1u, "one assembly collected");
    checks.Ok(data.fromOffline, "data is flagged as offline");
    checks.Equal(static_cast<uint64_t>(data.metadataVersion * 10.0), 290u, "version is 29.0");

    const er2::CollectedType* derived = FindType(data, "Derived");
    if (derived == nullptr)
    {
        checks.Ok(false, "Smoke.Derived was collected");
        return;
    }

    checks.Equal(derived->namespaceName, "Smoke", "Derived namespace");
    checks.Equal(derived->accessModifier, "public", "Derived is public");
    checks.Ok(derived->isSealed, "Derived is sealed");
    checks.Ok(derived->isSerializable, "Derived is serializable");
    checks.Equal(derived->parentName, "BaseType", "Derived base class resolved");
    checks.Equal(derived->interfaces.size(), 1u, "Derived has one interface");
    if (!derived->interfaces.empty())
    {
        checks.Equal(derived->interfaces[0], "IDisposable", "interface name resolved");
    }
    checks.Equal(derived->typeDefIndex, static_cast<uint64_t>(kTypeDerived), "TypeDefIndex is global");
    checks.Equal(derived->events.size(), 1u, "Derived has one event");
    checks.Equal(derived->properties.size(), 1u, "Derived has one property");
    checks.Equal(derived->attributes.size(), 1u, "Derived carries its custom attribute");
    if (!derived->attributes.empty())
    {
        checks.Equal(derived->attributes[0], "[Marker(\"tagged\")]", "v29 attribute blob decoded");
    }

    const er2::CollectedField* counter = FindField(*derived, "_counter");
    checks.Ok(counter != nullptr, "_counter field collected");
    if (counter != nullptr)
    {
        checks.Equal(counter->accessModifier, "private", "_counter is private");
        checks.Equal(counter->typeName, "int", "_counter uses the C# keyword");
        checks.Equal(counter->offset, 0x10u, "_counter offset from fieldOffsets");
        checks.Ok(!counter->isStatic, "_counter is not static");
    }

    const er2::CollectedField* tag = FindField(*derived, "Tag");
    checks.Ok(tag != nullptr, "Tag field collected");
    if (tag != nullptr)
    {
        checks.Equal(tag->accessModifier, "public", "Tag is public");
        checks.Ok(tag->isReadOnly, "Tag is readonly");
        checks.Equal(tag->typeName, "string", "Tag type resolved");
        checks.Equal(tag->offset, 0x18u, "Tag offset from fieldOffsets");
    }

    const er2::CollectedField* answer = FindField(*derived, "Answer");
    checks.Ok(answer != nullptr, "Answer field collected");
    if (answer != nullptr)
    {
        checks.Ok(answer->isLiteral, "Answer is a literal");
        checks.Equal(answer->defaultValue, "42", "compressed int default decoded");
        checks.Ok(!answer->defaultValueIsComment, "default rendered as a literal");
    }

    const er2::CollectedField* values = FindField(*derived, "Values");
    checks.Ok(values != nullptr, "Values field collected");
    if (values != nullptr)
    {
        checks.Equal(values->typeName, "int[]", "SZARRAY field resolved");
    }
    const er2::CollectedField* matrix = FindField(*derived, "Matrix");
    checks.Ok(matrix != nullptr, "Matrix field collected");
    if (matrix != nullptr)
    {
        checks.Equal(matrix->typeName, "int[,]", "ARRAY rank is preserved");
    }
    const er2::CollectedField* genericValue = FindField(*derived, "GenericValue");
    checks.Ok(genericValue != nullptr, "GenericValue field collected");
    if (genericValue != nullptr)
    {
        checks.Equal(genericValue->typeName, "Container<int>", "v27+ generic class pointer resolved");
    }

    const er2::CollectedMethod* run = FindMethod(*derived, "Run");
    checks.Ok(run != nullptr, "Derived.Run collected");
    if (run != nullptr)
    {
        checks.Equal(run->slot, 0u, "Derived.Run keeps its vtable slot");
        checks.Ok(run->isSealed && run->isVirtual && run->isOverride, "Derived.Run is sealed override");
        checks.Equal(run->rva, module.MethodRva(kMethodDerivedRun), "Run RVA");
        checks.Equal(run->fileOffset, module.MethodFileOffset(kMethodDerivedRun), "Run file offset");
        checks.Equal(run->address, module.MethodVa(kMethodDerivedRun), "Run VA");
    }

    const er2::CollectedMethod* compute = FindMethod(*derived, "Compute");
    checks.Ok(compute != nullptr, "Compute collected");
    if (compute != nullptr)
    {
        checks.Ok(compute->isStatic, "Compute is static");
        checks.Equal(compute->returnType, "int", "Compute returns int");
        checks.Equal(compute->params.size(), 5u, "Compute has five params");
        if (compute->params.size() == 5)
        {
            checks.Equal(compute->params[0].typeName, "int", "plain parameter type");
            checks.Ok(compute->params[1].isOut, "out parameter classified");
            checks.Ok(compute->params[2].isByRef, "ref parameter classified");
            checks.Ok(compute->params[3].isIn, "in parameter classified");
            checks.Equal(compute->params[4].defaultValue, "\"hi\"", "string default decoded");
        }
    }

    const er2::CollectedMethod* echo = FindMethod(*derived, "Echo<U>");
    checks.Ok(echo != nullptr, "generic Echo<U> collected");
    if (echo != nullptr)
    {
        checks.Equal(echo->returnType, "U", "MVAR return resolves to U");
        checks.Equal(echo->params.size(), 1u, "Echo has one parameter");
        if (!echo->params.empty())
        {
            checks.Equal(echo->params[0].typeName, "U", "MVAR parameter resolves to U");
        }
    }

    const er2::CollectedType* base = FindType(data, "BaseType");
    checks.Ok(base != nullptr, "BaseType collected");
    if (base != nullptr)
    {
        checks.Ok(base->isAbstract, "BaseType is abstract");
        checks.Equal(base->parentName, "", "System.Object base is omitted");
        const er2::CollectedMethod* abstractRun = FindMethod(*base, "Run");
        checks.Ok(abstractRun != nullptr, "BaseType.Run collected");
        if (abstractRun != nullptr)
        {
            checks.Ok(abstractRun->isAbstract, "BaseType.Run is abstract");
            checks.Equal(abstractRun->address, 0u, "abstract methods have no address");
        }
    }

    const er2::CollectedType* container = FindType(data, "Container<T>");
    checks.Ok(container != nullptr, "Container<T> collected");
    if (container != nullptr)
    {
        const er2::CollectedMethod* get = FindMethod(*container, "Get");
        checks.Ok(get != nullptr, "Container.Get collected");
        if (get != nullptr)
        {
            checks.Equal(get->returnType, "T", "VAR return resolves to T");
            checks.Equal(get->genericInstGroups.size(), 1u, "one GenericInstMethod group");
            if (!get->genericInstGroups.empty() && !get->genericInstGroups[0].entries.empty())
            {
                checks.Equal(get->genericInstGroups[0].entries[0], "Container<int>.Get", "generic instantiation named");
            }
        }
    }

    const er2::CollectedType* nested = FindType(data, "Derived.Nested");
    checks.Ok(nested != nullptr, "nested type collected with dotted name");
    if (nested != nullptr)
    {
        checks.Equal(nested->accessModifier, "protected internal", "nested visibility uses legal C# words");
        const er2::CollectedField* missing = FindField(*nested, "UnknownOffset");
        checks.Ok(missing != nullptr && !missing->hasOffset, "missing field offset is preserved");
    }

    const er2::CollectedType* disposable = FindType(data, "IDisposable");
    checks.Ok(disposable != nullptr && disposable->kind == er2::TypeKind::Interface, "interface kind resolved");
    checks.Equal(data.strings.size(), 2u, "both string literals collected");
    if (data.strings.size() == 2)
    {
        checks.Equal(data.strings[0].value, "hello smoke", "first literal decoded");
        checks.Equal(data.strings[1].value, "second literal", "second literal decoded");
    }
}

} // namespace OfflineBehavior
