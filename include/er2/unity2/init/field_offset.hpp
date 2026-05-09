#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "metadata.hpp"

#include "../dumpsdk/sdk_common.hpp"
#include "../dumpsdk/sdk_metadata_helpers.hpp"
#include "../dumpsdk/sdk_registration.hpp"

namespace er2
{

namespace detail_field_offset
{

using FieldsByType = std::unordered_map<std::string, std::unordered_map<std::string, std::uint32_t>>;

struct RuntimeState
{
    bool loaded = false;
    std::uint32_t pid = 0;
    std::uintptr_t gameAssemblyBase = 0;
    std::uintptr_t metadataRegistration = 0;
    std::uintptr_t fieldOffsetsPtr = 0;
    FieldsByType fieldsByType;
    std::unordered_map<std::string, std::string> aliasToFullName;
    std::unordered_map<std::string, std::string> parentByType;
    std::unordered_set<std::string> knownTypes;
};

// NOTE: Not thread-safe. All access must occur on a single thread.
inline RuntimeState g_fieldOffsetState;

inline void ResetRuntimeState()
{
    g_fieldOffsetState = RuntimeState{};
}

inline std::string BuildShortTypeName(const std::string& fullName, const std::string& namespaceName)
{
    if (!namespaceName.empty())
    {
        const std::string prefix = namespaceName + ".";
        if (fullName.rfind(prefix, 0) == 0)
        {
            return fullName.substr(prefix.size());
        }
    }

    return fullName;
}

inline std::string ResolveTypeKind(
    const DumpSdk6TypeDefRaw& typeDef,
    const std::unordered_map<std::uint32_t, std::string>& byvalToFullName)
{
    if ((typeDef.flags & 0x20u) != 0u)
    {
        return "interface";
    }

    if (typeDef.parentIndex < 0)
    {
        return "class";
    }

    const auto parentIt = byvalToFullName.find(static_cast<std::uint32_t>(typeDef.parentIndex));
    if (parentIt == byvalToFullName.end())
    {
        return "class";
    }

    if (parentIt->second == "System.Enum")
    {
        return "enum";
    }

    if (parentIt->second == "System.ValueType")
    {
        return "struct";
    }

    return "class";
}

inline void AddAliasCandidate(
    const std::string& alias,
    const std::string& fullName,
    std::unordered_map<std::string, std::string>& aliasToFullName,
    std::unordered_set<std::string>& ambiguousAliases)
{
    if (alias.empty() || alias == fullName)
    {
        return;
    }

    if (ambiguousAliases.find(alias) != ambiguousAliases.end())
    {
        return;
    }

    const auto existing = aliasToFullName.find(alias);
    if (existing == aliasToFullName.end())
    {
        aliasToFullName.emplace(alias, fullName);
        return;
    }

    if (existing->second != fullName)
    {
        aliasToFullName.erase(existing);
        ambiguousAliases.insert(alias);
    }
}

inline bool BuildRuntimeState(RuntimeState& out)
{
    out = RuntimeState{};

    if (!IsInited() || Runtime() != ManagedBackend::Il2Cpp)
    {
        return false;
    }

    const std::optional<ModuleInfo> gameAssembly = TryGetGameAssemblyModuleInfo();
    if (!gameAssembly || !gameAssembly->base)
    {
        return false;
    }

    const IMemoryAccessor& mem = Mem();

    MetadataHint hint;
    if (!BuildMetadataHintTScore(mem, gameAssembly->base, Pid(), L"", L"GameAssembly.dll", hint))
    {
        return false;
    }

    if (!hint.metadataRegistration)
    {
        return false;
    }

    std::vector<std::uint8_t> metaBytes;
    if (!ExportMetadataByScore(mem, gameAssembly->base, 0x200000u, 8192, 15.0, false, 0, 0x200000u, metaBytes))
    {
        return false;
    }

    MetadataHeaderFields header;
    if (!ReadMetadataHeaderFieldsFromBytes(metaBytes, header))
    {
        return false;
    }

    std::uintptr_t typesPtr = 0;
    std::uint32_t typesCount = 0;
    std::uintptr_t fieldOffsetsPtr = 0;
    if (!DumpSdk6GetMetadataRegistrationTypes(
            mem,
            hint.metadataRegistration,
            header.version,
            typesPtr,
            typesCount,
            fieldOffsetsPtr))
    {
        return false;
    }

    (void)typesPtr;
    (void)typesCount;

    if (!fieldOffsetsPtr)
    {
        return false;
    }

    std::vector<std::string> typeFullName;
    std::unordered_map<std::uint32_t, std::string> byvalToFullName;
    if (!BuildTypeFullNameAndByvalMapFromBytes(metaBytes, header, typeFullName, byvalToFullName))
    {
        return false;
    }

    const std::uint32_t typeDefCount =
        header.typeDefinitionsSize / static_cast<std::uint32_t>(sizeof(DumpSdk6TypeDefRaw));
    const std::uint32_t fieldDefCount =
        header.fieldsSize / static_cast<std::uint32_t>(sizeof(DumpSdk6FieldDefRaw));
    if (typeDefCount == 0 || fieldDefCount == 0)
    {
        return false;
    }

    out.fieldsByType.reserve(typeDefCount);
    std::unordered_set<std::string> ambiguousAliases;

    for (std::uint32_t typeIndex = 0; typeIndex < typeDefCount; ++typeIndex)
    {
        const std::size_t typeBase =
            static_cast<std::size_t>(header.typeDefinitionsOffset) +
            static_cast<std::size_t>(typeIndex) * sizeof(DumpSdk6TypeDefRaw);
        if (typeBase + sizeof(DumpSdk6TypeDefRaw) > metaBytes.size())
        {
            break;
        }

        const DumpSdk6TypeDefRaw* typeDef =
            reinterpret_cast<const DumpSdk6TypeDefRaw*>(metaBytes.data() + typeBase);
        if (typeIndex >= typeFullName.size() || typeFullName[typeIndex].empty())
        {
            continue;
        }

        std::string namespaceName;
        (void)ReadCStringFromMetadataBytes(metaBytes, header, typeDef->namespaceIndex, namespaceName);

        const std::string& fullName = typeFullName[typeIndex];
        const std::string shortName = BuildShortTypeName(fullName, namespaceName);
        const std::string kind = ResolveTypeKind(*typeDef, byvalToFullName);
        out.knownTypes.insert(fullName);
        AddAliasCandidate(shortName, fullName, out.aliasToFullName, ambiguousAliases);

        if (typeDef->parentIndex >= 0)
        {
            const auto parentIt = byvalToFullName.find(static_cast<std::uint32_t>(typeDef->parentIndex));
            if (parentIt != byvalToFullName.end() && !parentIt->second.empty())
            {
                out.parentByType.emplace(fullName, parentIt->second);
            }
        }

        if (typeDef->fieldCount == 0 || typeDef->fieldStart < 0)
        {
            continue;
        }

        std::uintptr_t fieldOffsetArray = 0;
        if (!ReadPtr(mem, fieldOffsetsPtr + static_cast<std::uintptr_t>(typeIndex) * 8u, fieldOffsetArray) ||
            fieldOffsetArray == 0)
        {
            continue;
        }

        std::unordered_map<std::string, std::uint32_t> classFields;
        classFields.reserve(typeDef->fieldCount);

        for (std::uint32_t fieldSlot = 0; fieldSlot < typeDef->fieldCount; ++fieldSlot)
        {
            const std::uint32_t fieldIndex = static_cast<std::uint32_t>(typeDef->fieldStart) + fieldSlot;
            if (fieldIndex >= fieldDefCount)
            {
                break;
            }

            const std::size_t fieldBase =
                static_cast<std::size_t>(header.fieldsOffset) +
                static_cast<std::size_t>(fieldIndex) * sizeof(DumpSdk6FieldDefRaw);
            if (fieldBase + sizeof(DumpSdk6FieldDefRaw) > metaBytes.size())
            {
                break;
            }

            const DumpSdk6FieldDefRaw* fieldDef =
                reinterpret_cast<const DumpSdk6FieldDefRaw*>(metaBytes.data() + fieldBase);

            std::string fieldName;
            (void)ReadCStringFromMetadataBytes(metaBytes, header, fieldDef->nameIndex, fieldName);
            if (fieldName.empty())
            {
                continue;
            }

            std::int32_t rawOffset = 0;
            if (!ReadValue(mem, fieldOffsetArray + static_cast<std::uintptr_t>(fieldSlot) * 4u, rawOffset))
            {
                continue;
            }

            if (rawOffset < 0)
            {
                continue;
            }

            std::uint32_t resolvedOffset = static_cast<std::uint32_t>(rawOffset);
            if ((kind == "struct" || kind == "enum") && resolvedOffset >= 0x10u)
            {
                resolvedOffset -= 0x10u;
            }

            classFields[fieldName] = resolvedOffset;
        }

        if (classFields.empty())
        {
            continue;
        }

        out.fieldsByType.emplace(fullName, std::move(classFields));
    }

    if (out.fieldsByType.empty())
    {
        return false;
    }

    out.loaded = true;
    out.pid = Pid();
    out.gameAssemblyBase = gameAssembly->base;
    out.metadataRegistration = hint.metadataRegistration;
    out.fieldOffsetsPtr = fieldOffsetsPtr;
    return true;
}

inline bool EnsureRuntimeState()
{
    if (!IsInited() || Runtime() != ManagedBackend::Il2Cpp)
    {
        ResetRuntimeState();
        return false;
    }

    const std::optional<ModuleInfo> gameAssembly = TryGetGameAssemblyModuleInfo();
    if (!gameAssembly || !gameAssembly->base)
    {
        ResetRuntimeState();
        return false;
    }

    if (g_fieldOffsetState.loaded && g_fieldOffsetState.pid == Pid() &&
        g_fieldOffsetState.gameAssemblyBase == gameAssembly->base)
    {
        return true;
    }

    RuntimeState built;
    if (!BuildRuntimeState(built))
    {
        ResetRuntimeState();
        return false;
    }

    g_fieldOffsetState = std::move(built);
    return true;
}

inline bool TryResolveFullTypeName(const std::string& typeName, std::string& outFullName)
{
    outFullName.clear();

    if (!EnsureRuntimeState())
    {
        return false;
    }

    if (g_fieldOffsetState.fieldsByType.find(typeName) != g_fieldOffsetState.fieldsByType.end())
    {
        outFullName = typeName;
        return true;
    }

    if (g_fieldOffsetState.knownTypes.find(typeName) != g_fieldOffsetState.knownTypes.end())
    {
        outFullName = typeName;
        return true;
    }

    const auto aliasIt = g_fieldOffsetState.aliasToFullName.find(typeName);
    if (aliasIt == g_fieldOffsetState.aliasToFullName.end())
    {
        return false;
    }

    outFullName = aliasIt->second;
    return true;
}

} // namespace detail_field_offset

inline bool SupportsDynamicFieldOffsets()
{
    return IsInited() && Supports(Runtime(), Feature::Il2CppMetadata);
}

inline bool EnsureFieldOffsetsInited()
{
    return detail_field_offset::EnsureRuntimeState();
}

inline bool TryGetFieldOffset(
    const std::string& typeName,
    const std::string& fieldName,
    std::uint32_t& outOffset)
{
    outOffset = 0;

    std::string fullTypeName;
    if (!detail_field_offset::TryResolveFullTypeName(typeName, fullTypeName))
    {
        return false;
    }

    std::unordered_set<std::string> visitedTypes;
    std::string currentType = fullTypeName;

    while (!currentType.empty() && visitedTypes.insert(currentType).second)
    {
        const auto typeIt = detail_field_offset::g_fieldOffsetState.fieldsByType.find(currentType);
        if (typeIt != detail_field_offset::g_fieldOffsetState.fieldsByType.end())
        {
            const auto fieldIt = typeIt->second.find(fieldName);
            if (fieldIt != typeIt->second.end())
            {
                outOffset = fieldIt->second;
                return true;
            }
        }

        const auto parentIt = detail_field_offset::g_fieldOffsetState.parentByType.find(currentType);
        if (parentIt == detail_field_offset::g_fieldOffsetState.parentByType.end())
        {
            break;
        }

        currentType = parentIt->second;
    }

    return false;
}

inline std::uint32_t GetFieldOffsetOr(
    const std::string& typeName,
    const std::string& fieldName,
    std::uint32_t fallback)
{
    std::uint32_t outOffset = 0;
    if (!TryGetFieldOffset(typeName, fieldName, outOffset))
    {
        return fallback;
    }

    return outOffset;
}

inline std::uintptr_t FieldOffsetsMetadataRegistrationVa()
{
    if (!EnsureFieldOffsetsInited())
    {
        return 0;
    }

    return detail_field_offset::g_fieldOffsetState.metadataRegistration;
}

inline std::uintptr_t FieldOffsetsTableVa()
{
    if (!EnsureFieldOffsetsInited())
    {
        return 0;
    }

    return detail_field_offset::g_fieldOffsetState.fieldOffsetsPtr;
}

} // namespace er2

