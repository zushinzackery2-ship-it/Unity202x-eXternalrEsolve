#include <er2/unity2/dumpsdk/offline/OfflineCollector.h>

#include "OfflineCollectorInternal.h"

#include <er2/os/win/local_memory_accessor.hpp>
#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>
#include <er2/unity2/metadata.hpp>

#include <format>
#include <limits>

namespace er2
{

namespace
{

int CountMethodsForSearch(const Metadata& metadata)
{
    if (metadata.Version() > 24.1)
    {
        return static_cast<int>(metadata.MethodDefs().size());
    }
    int count = 0;
    for (const Il2CppMethodDefinition& method : metadata.MethodDefs())
    {
        if (method.methodIndex >= 0)
        {
            ++count;
        }
    }
    return count;
}

bool TryFindRegistrationsWithEr2(
    uintptr_t moduleBase,
    uint32_t moduleSize,
    uintptr_t metadataBase,
    uintptr_t& codeRegistration,
    uintptr_t& metadataRegistration)
{
    codeRegistration = 0;
    metadataRegistration = 0;
    if (moduleBase == 0 || moduleSize == 0 || metadataBase == 0)
    {
        return false;
    }

    LocalMemoryAccessor memory;
    Il2CppRegs registrations{};
    const bool found = FindIl2CppRegistrations(
        memory,
        moduleBase,
        moduleSize,
        {},
        metadataBase,
        0x200000u,
        45.0,
        registrations);
    codeRegistration = registrations.codeRegistration;
    metadataRegistration = registrations.metadataRegistration;
    DumpSdkLog(DumpSdkLogLevel::Info, std::format(
        "[Il2CppOffline] er2 registrations found={} cr=0x{:X} mr=0x{:X}",
        found,
        codeRegistration,
        metadataRegistration));
    return codeRegistration != 0 && metadataRegistration != 0;
}

} // namespace

bool Collect(
    uintptr_t moduleBase,
    uint32_t moduleSize,
    const uint8_t* metaBytes,
    size_t metaSize,
    uintptr_t metaBase,
    CollectedData& out,
    std::string& error,
    RegistrationInitResult* registrationOut)
{
    if (moduleBase == 0 || moduleSize == 0 || metaBytes == nullptr || metaSize == 0)
    {
        error = "invalid Collect input";
        return false;
    }

    PeImage pe{};
    if (!LoadPeImageFromModuleRange(moduleBase, moduleSize, pe, error))
    {
        return false;
    }

    return Collect(
        pe,
        metaBytes,
        metaSize,
        metaBase,
        out,
        error,
        registrationOut);
}

bool Collect(
    const PeImage& pe,
    const uint8_t* metaBytes,
    size_t metaSize,
    uintptr_t metaBase,
    CollectedData& out,
    std::string& error,
    RegistrationInitResult* registrationOut)
{
    out = {};
    if (!pe.IsBound()
        || pe.ImageBase() == 0
        || pe.Size() == 0
        || pe.Size() > (std::numeric_limits<std::uint32_t>::max)()
        || metaBytes == nullptr
        || metaSize == 0)
    {
        error = "invalid Collect snapshot input";
        return false;
    }

    const uintptr_t moduleBase = static_cast<uintptr_t>(pe.ImageBase());
    const uint32_t moduleSize = static_cast<uint32_t>(pe.Size());

    Metadata metadata;
    try
    {
        metadata.Load(metaBytes, metaSize);
    }
    catch (const std::exception& ex)
    {
        error = std::string("metadata parse failed: ") + ex.what();
        return false;
    }

    RegistrationSearchInput searchInput{};
    searchInput.methodCount = CountMethodsForSearch(metadata);
    searchInput.typeDefCount = static_cast<int>(metadata.TypeDefs().size());
    searchInput.imageCount = static_cast<int>(metadata.ImageDefs().size());
    searchInput.metadataUsagesCount = metadata.MetadataUsagesCount();
    searchInput.version = metadata.Version();

    RegistrationSearch search(pe, searchInput);
    RegistrationInitResult registration{};
    uintptr_t codeRegistration = 0;
    uintptr_t metadataRegistration = 0;
    bool ready = false;
    if (TryFindRegistrationsWithEr2(
            moduleBase,
            moduleSize,
            metaBase,
            codeRegistration,
            metadataRegistration))
    {
        ready = search.InitFromAddresses(
            codeRegistration,
            metadataRegistration,
            registration,
            error);
        if (!ready)
        {
            DumpSdkLog(DumpSdkLogLevel::Warn,
                "[Il2CppOffline] primary registration addresses rejected: " + error);
            error.clear();
        }
    }
    if (!ready && !search.FindAndInit(registration, error))
    {
        return false;
    }
    if (registrationOut != nullptr)
    {
        *registrationOut = registration;
    }

    OfflineRuntimeContext runtime(pe, metadata, metaBase);
    if (!runtime.Init(registration, error))
    {
        return false;
    }
    TypeNameResolver resolver(metadata, runtime);
    DefaultValueDecoder decoder(metadata, runtime, resolver);
    CustomAttributeReader attributes(metadata, runtime, resolver, decoder);
    const CollectContext context{ metadata, runtime, resolver, decoder, attributes };

    out.metadataVersion = runtime.Version();
    out.imageBase = static_cast<uintptr_t>(pe.ImageBase());
    out.fromOffline = true;

    const std::vector<Il2CppTypeDefinition>& typeDefs = metadata.TypeDefs();
    for (size_t imageIndex = 0; imageIndex < metadata.ImageDefs().size(); ++imageIndex)
    {
        const Il2CppImageDefinition& imageDef = metadata.ImageDefs()[imageIndex];
        const std::string imageName = metadata.GetStringFromIndex(imageDef.nameIndex);
        CollectedAssembly assembly{};
        assembly.name = imageName;
        assembly.fileName = imageName;
        assembly.imageIndex = imageIndex;
        assembly.typeStartIndex = imageDef.typeStart >= 0
            ? static_cast<size_t>(imageDef.typeStart)
            : 0;

        const int64_t end = static_cast<int64_t>(imageDef.typeStart) + imageDef.typeCount;
        for (int64_t typeDefIndex = imageDef.typeStart; typeDefIndex < end; ++typeDefIndex)
        {
            if (typeDefIndex < 0 || static_cast<size_t>(typeDefIndex) >= typeDefs.size())
            {
                continue;
            }
            const Il2CppTypeDefinition& typeDef = typeDefs[static_cast<size_t>(typeDefIndex)];
            CollectedType type{};
            type.name = resolver.GetTypeDefName(typeDef, false, true);
            type.namespaceName = metadata.GetStringFromIndex(typeDef.namespaceIndex);
            type.assemblyName = assembly.name;
            type.token = typeDef.token;
            type.flags = typeDef.flags;
            type.kind = ResolveTypeKind(typeDef);
            type.index = static_cast<size_t>(typeDefIndex);
            type.typeDefIndex = static_cast<size_t>(typeDefIndex);
            type.isNested = typeDef.declaringTypeIndex != -1;
            type.isGeneric = typeDef.genericContainerIndex >= 0;
            type.isAbstract = (typeDef.flags & kTypeAbstract) != 0;
            type.isSealed = (typeDef.flags & kTypeSealed) != 0;
            type.isStaticClass = type.isAbstract && type.isSealed;
            type.isPublic = (typeDef.flags & kTypeVisibilityMask) == kTypePublic ||
                (typeDef.flags & kTypeVisibilityMask) == kTypeNestedPublic;
            type.isSerializable = (typeDef.flags & kTypeSerializable) != 0;
            type.accessModifier = AccessFromTypeFlags(typeDef.flags);
            type.attributes = attributes.Render(
                imageIndex,
                typeDef.customAttributeIndex,
                typeDef.token);

            CollectTypeHierarchy(context, typeDef, type);
            CollectFields(context, imageIndex, static_cast<size_t>(typeDefIndex), typeDef, type);
            CollectProperties(context, imageIndex, typeDef, type);
            CollectEvents(context, imageIndex, typeDef, type);
            CollectMethods(context, imageIndex, imageName, typeDef, type);
            assembly.types.push_back(std::move(type));
        }
        out.assemblies.push_back(std::move(assembly));
    }

    CollectStringLiteralsAndUsages(context, out);
    if (out.assemblies.empty())
    {
        error = "offline collect produced zero assemblies";
        return false;
    }
    DumpSdkLog(DumpSdkLogLevel::Info, std::format(
        "[Il2CppOffline] Collect complete assemblies={} strings={} metadata={} methodRefs={}",
        out.assemblies.size(),
        out.strings.size(),
        out.metadata.size(),
        out.methodRefs.size()));
    return true;
}

} // namespace er2
