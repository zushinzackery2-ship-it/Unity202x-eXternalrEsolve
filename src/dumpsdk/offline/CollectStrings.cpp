#include "OfflineCollectorInternal.h"

#include <er2/unity2/dumpsdk/dump_progress.hpp>

namespace er2
{

void CollectStringLiteralsAndUsages(const CollectContext& context, CollectedData& out)
{
    const std::vector<uintptr_t>& usages = context.runtime.MetadataUsageAddresses();
    const auto& usageMap = context.metadata.MetadataUsages();
    auto slotAddress = [&](uint32_t destinationIndex)
    {
        return destinationIndex >= usages.size()
            ? uintptr_t{ 0 }
            : context.runtime.MetadataUsageSlotAddress(destinationIndex);
    };

    const auto literalEntries = usageMap.find(
        static_cast<uint32_t>(Il2CppMetadataUsage::StringLiteral));
    auto usageCount = [&](Il2CppMetadataUsage kind)
    {
        const auto entries = usageMap.find(static_cast<uint32_t>(kind));
        return entries == usageMap.end() ? size_t{ 0 } : entries->second.size();
    };
    const bool useLiteralFallback = literalEntries == usageMap.end()
        || literalEntries->second.empty();
    const size_t totalUsageEntries =
        usageCount(Il2CppMetadataUsage::StringLiteral)
        + usageCount(Il2CppMetadataUsage::TypeInfo)
        + usageCount(Il2CppMetadataUsage::Il2CppType)
        + usageCount(Il2CppMetadataUsage::MethodDef)
        + usageCount(Il2CppMetadataUsage::FieldInfo)
        + usageCount(Il2CppMetadataUsage::MethodRef)
        + (useLiteralFallback ? context.metadata.StringLiterals().size() : 0);
    DumpSdkProgressScope progress("Collect metadata usages", totalUsageEntries);
    size_t processedEntries = 0;
    auto advanceProgress = [&]()
    {
        progress.Update(++processedEntries);
    };

    if (literalEntries != usageMap.end())
    {
        for (const auto& [destination, source] : literalEntries->second)
        {
            advanceProgress();
            out.strings.push_back({
                context.metadata.GetStringLiteralFromIndex(source),
                slotAddress(destination)
            });
        }
    }

    const std::vector<Il2CppTypeDefinition>& typeDefs = context.metadata.TypeDefs();
    const std::vector<Il2CppMethodDefinition>& methodDefs = context.metadata.MethodDefs();
    const std::vector<Il2CppFieldRef>& fieldRefs = context.metadata.FieldRefs();
    const std::vector<Il2CppFieldDefinition>& fieldDefs = context.metadata.FieldDefs();

    auto methodFullName = [&](uint32_t methodIndex)
    {
        if (methodIndex >= methodDefs.size())
        {
            return std::string{};
        }
        const Il2CppMethodDefinition& method = methodDefs[methodIndex];
        if (method.declaringType < 0 || static_cast<size_t>(method.declaringType) >= typeDefs.size())
        {
            return std::string{};
        }
        return context.resolver.GetTypeDefName(
            typeDefs[static_cast<size_t>(method.declaringType)],
            true,
            false) + "." + context.metadata.GetStringFromIndex(method.nameIndex) + "()";
    };

    auto emitMetadata = [&](Il2CppMetadataUsage kind, const char* suffix)
    {
        const auto entries = usageMap.find(static_cast<uint32_t>(kind));
        if (entries == usageMap.end())
        {
            return;
        }
        for (const auto& [destination, source] : entries->second)
        {
            advanceProgress();
            CollectedMetadata metadata{};
            metadata.address = slotAddress(destination);
            if (metadata.address == 0)
            {
                continue;
            }
            if (kind == Il2CppMetadataUsage::TypeInfo || kind == Il2CppMetadataUsage::Il2CppType)
            {
                const Il2CppTypeRuntime* type = context.runtime.GetTypeByIndex(source);
                if (type == nullptr)
                {
                    continue;
                }
                metadata.name = context.resolver.GetTypeName(*type, true, false) + suffix;
                metadata.signature = kind == Il2CppMetadataUsage::TypeInfo
                    ? "struct Il2CppClass*"
                    : "struct Il2CppType*";
            }
            else if (kind == Il2CppMetadataUsage::MethodDef)
            {
                metadata.name = methodFullName(source);
                if (metadata.name.empty())
                {
                    continue;
                }
                metadata.name += suffix;
                metadata.signature = "struct MethodInfo*";
            }
            else if (kind == Il2CppMetadataUsage::FieldInfo)
            {
                if (source >= fieldRefs.size())
                {
                    continue;
                }
                const Il2CppFieldRef& fieldRef = fieldRefs[source];
                const Il2CppTypeRuntime* type = context.runtime.GetTypeByIndex(fieldRef.typeIndex);
                Il2CppTypeDefinition typeDef{};
                if (type == nullptr || !context.resolver.TryGetTypeDefinition(*type, typeDef))
                {
                    continue;
                }
                const int64_t flatIndex = static_cast<int64_t>(typeDef.fieldStart) + fieldRef.fieldIndex;
                if (flatIndex < 0 || static_cast<size_t>(flatIndex) >= fieldDefs.size())
                {
                    continue;
                }
                metadata.name = context.resolver.GetTypeName(*type, true, false) + "." +
                    context.metadata.GetStringFromIndex(
                        fieldDefs[static_cast<size_t>(flatIndex)].nameIndex) + suffix;
                metadata.signature = "struct FieldInfo*";
            }
            out.metadata.push_back(std::move(metadata));
        }
    };

    emitMetadata(Il2CppMetadataUsage::TypeInfo, "_TypeInfo");
    emitMetadata(Il2CppMetadataUsage::Il2CppType, "_var");
    emitMetadata(Il2CppMetadataUsage::MethodDef, "_MethodInfo");
    emitMetadata(Il2CppMetadataUsage::FieldInfo, "_FieldInfo");

    const auto methodRefs = usageMap.find(static_cast<uint32_t>(Il2CppMetadataUsage::MethodRef));
    if (methodRefs != usageMap.end())
    {
        for (const auto& [destination, source] : methodRefs->second)
        {
            advanceProgress();
            Il2CppMethodSpec spec{};
            std::string typeName;
            std::string methodName;
            const uintptr_t slot = slotAddress(destination);
            if (slot == 0 ||
                !context.runtime.TryGetMethodSpec(static_cast<int32_t>(source), spec) ||
                !context.resolver.GetMethodSpecName(spec, typeName, methodName))
            {
                continue;
            }
            out.methodRefs.push_back({
                typeName + "." + methodName + "_MethodInfo",
                slot,
                context.runtime.GetGenericMethodPointerForSpec(static_cast<int32_t>(source))
            });
        }
    }

    if (out.strings.empty())
    {
        for (size_t i = 0; i < context.metadata.StringLiterals().size(); ++i)
        {
            advanceProgress();
            out.strings.push_back({
                context.metadata.GetStringLiteralFromIndex(static_cast<uint32_t>(i)),
                0
            });
        }
    }
    progress.Complete();
}

} // namespace er2
