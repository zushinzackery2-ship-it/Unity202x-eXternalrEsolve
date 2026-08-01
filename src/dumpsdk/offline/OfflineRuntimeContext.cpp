#include <er2/unity2/dumpsdk/offline/OfflineRuntimeContext.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>

#include <format>

namespace er2
{

OfflineRuntimeContext::OfflineRuntimeContext(
    const PeImage& pe,
    const Metadata& metadata,
    uintptr_t metadataVirtualAddress)
    : pe_(pe)
    , metadata_(metadata)
    , metadataVirtualAddress_(metadataVirtualAddress)
    , version_(metadata.Version())
{
}

bool OfflineRuntimeContext::Init(
    const RegistrationInitResult& registration,
    std::string& error)
{
    if (!registration.success)
    {
        error = "registration search result is not initialized";
        return false;
    }

    version_ = registration.resolvedVersion > 0.0
        ? registration.resolvedVersion
        : metadata_.Version();
    codeRegistrationVa_ = registration.codeRegistrationVa;
    metadataRegistrationVa_ = registration.metadataRegistrationVa;

    CodeRegistrationView codeReg{};
    if (!ReadCodeRegistration(pe_, codeRegistrationVa_, version_, codeReg))
    {
        error = "failed to read CodeRegistration";
        return false;
    }
    MetadataRegistrationView metaReg{};
    if (!ReadMetadataRegistration(pe_, metadataRegistrationVa_, version_, metaReg))
    {
        error = "failed to read MetadataRegistration";
        return false;
    }
    if (metaReg.types == 0 || metaReg.typesCount <= 0)
    {
        error = "MetadataRegistration has no types";
        return false;
    }

    isDumped_ = version_ >= 27.0 && metadataVirtualAddress_ != 0;
    if (!LoadTypes(metaReg, error))
    {
        return false;
    }
    if (!LoadFieldOffsets(metaReg))
    {
        error = "failed to read fieldOffsets";
        return false;
    }
    if (!LoadMethodPointers(codeReg))
    {
        error = "failed to read method pointers";
        return false;
    }

    LoadGenericData(codeReg, metaReg);
    LoadMetadataUsages(metaReg);
    LoadCustomAttributeGenerators(codeReg);

    DumpSdkLog(DumpSdkLogLevel::Info, std::format(
        "[Il2CppOffline] runtime context version={} types={} modules={} genericInsts={} usages={}",
        version_,
        types_.size(),
        codeGenModuleMethodPointers_.size(),
        genericInsts_.size(),
        metadataUsages_.size()));
    return true;
}

uintptr_t OfflineRuntimeContext::MetadataUsageSlotAddress(uint32_t destinationIndex) const
{
    if (metadataUsagesVa_ == 0 || destinationIndex >= metadataUsages_.size())
    {
        return 0;
    }
    return metadataUsagesVa_ + static_cast<uintptr_t>(destinationIndex) * sizeof(uintptr_t);
}

bool OfflineRuntimeContext::TryGetMethodSpec(int32_t methodSpecIndex, Il2CppMethodSpec& out) const
{
    if (methodSpecIndex < 0 || static_cast<size_t>(methodSpecIndex) >= methodSpecs_.size())
    {
        return false;
    }
    out = methodSpecs_[static_cast<size_t>(methodSpecIndex)];
    return true;
}

uintptr_t OfflineRuntimeContext::GetGenericMethodPointerForSpec(int32_t methodSpecIndex) const
{
    if (methodSpecIndex < 0 || static_cast<size_t>(methodSpecIndex) >= methodSpecGenericPointers_.size())
    {
        return 0;
    }
    return methodSpecGenericPointers_[static_cast<size_t>(methodSpecIndex)];
}

uintptr_t OfflineRuntimeContext::GetCustomAttributeGenerator(int32_t attributeIndex) const
{
    if (attributeIndex < 0 || static_cast<size_t>(attributeIndex) >= customAttributeGenerators_.size())
    {
        return 0;
    }
    return customAttributeGenerators_[static_cast<size_t>(attributeIndex)];
}

const std::vector<OfflineRuntimeContext::MethodSpecEntry>*
OfflineRuntimeContext::GetMethodSpecsForMethod(int32_t methodDefinitionIndex) const
{
    const auto found = methodDefinitionMethodSpecs_.find(methodDefinitionIndex);
    return found == methodDefinitionMethodSpecs_.end() ? nullptr : &found->second;
}

} // namespace er2
