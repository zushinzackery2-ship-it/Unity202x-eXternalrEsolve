#pragma once

#include <er2/unity2/dumpsdk/offline/OfflineRuntimeContext.h>

#include <cstdint>
#include <string>

namespace er2
{

class TypeNameResolver
{
public:
    TypeNameResolver(const Metadata& metadata, const OfflineRuntimeContext& context);

    std::string GetTypeName(
        const Il2CppTypeRuntime& type,
        bool addNamespace,
        bool isNested) const;
    std::string GetTypeNameByIndex(int32_t typeIndex, bool addNamespace) const;
    std::string GetTypeDefName(
        const Il2CppTypeDefinition& typeDef,
        bool addNamespace,
        bool genericParameter) const;
    std::string GetGenericContainerParams(const Il2CppGenericContainer& container) const;
    std::string GetGenericInstParams(const Il2CppGenericInst& inst) const;

    bool GetMethodSpecName(
        const Il2CppMethodSpec& spec,
        std::string& typeName,
        std::string& methodName) const;

    bool TryGetTypeDefinition(const Il2CppTypeRuntime& type, Il2CppTypeDefinition& out) const;
    bool TryGetGenericParameter(const Il2CppTypeRuntime& type, Il2CppGenericParameter& out) const;

private:
    std::string PrimitiveName(Il2CppTypeEnum type) const;
    bool TryGetGenericClassTypeDefinition(
        const Il2CppGenericClass& genericClass,
        Il2CppTypeDefinition& out) const;

    const Metadata& metadata_;
    const OfflineRuntimeContext& context_;
};

} // namespace er2
