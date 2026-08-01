#pragma once

#include <er2/unity2/dumpsdk/collected_data.hpp>
#include <er2/unity2/dumpsdk/offline/CustomAttributeReader.h>

namespace er2
{

struct CollectContext
{
    const Metadata& metadata;
    const OfflineRuntimeContext& runtime;
    const TypeNameResolver& resolver;
    const DefaultValueDecoder& decoder;
    const CustomAttributeReader& attributes;
};

std::string AccessFromTypeFlags(uint32_t flags);
std::string AccessFromFieldFlags(uint32_t flags);
std::string AccessFromMethodFlags(uint32_t flags);
std::string MethodModifiersFromFlags(uint32_t flags);
TypeKind ResolveTypeKind(const Il2CppTypeDefinition& typeDef);

void CollectTypeHierarchy(
    const CollectContext& context,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out);
void CollectFields(
    const CollectContext& context,
    size_t imageIndex,
    size_t typeDefIndex,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out);
void CollectProperties(
    const CollectContext& context,
    size_t imageIndex,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out);
void CollectEvents(
    const CollectContext& context,
    size_t imageIndex,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out);
void CollectMethods(
    const CollectContext& context,
    size_t imageIndex,
    const std::string& imageName,
    const Il2CppTypeDefinition& typeDef,
    CollectedType& out);
void CollectStringLiteralsAndUsages(const CollectContext& context, CollectedData& out);

} // namespace er2
