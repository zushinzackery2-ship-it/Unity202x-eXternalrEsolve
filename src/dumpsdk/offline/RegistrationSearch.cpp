#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>

#include <format>

namespace er2
{

RegistrationSearch::RegistrationSearch(const PeImage& pe, const RegistrationSearchInput& input)
    : pe_(pe)
    , input_(input)
{
}

bool RegistrationSearch::FindAndInit(RegistrationInitResult& out, std::string& error)
{
    out = {};
    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] RegistrationSearch: find CodeRegistration");
    const uintptr_t codeRegistration = FindCodeRegistration();
    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] RegistrationSearch: CodeRegistration=0x{:X}", codeRegistration));

    DumpSdkLog(DumpSdkLogLevel::Info, "[Il2CppOffline] RegistrationSearch: find MetadataRegistration");
    const uintptr_t metadataRegistration = FindMetadataRegistration();
    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] RegistrationSearch: MetadataRegistration=0x{:X}", metadataRegistration));

    if (metadataRegistration == 0)
    {
        error = "MetadataRegistration not found";
        return false;
    }
    if (codeRegistration == 0)
    {
        error = "CodeRegistration not found";
        return false;
    }
    if (!AutoPlusInit(codeRegistration, metadataRegistration, out))
    {
        error = "AutoPlusInit failed for all registration candidates";
        return false;
    }
    out.success = true;
    return true;
}

bool RegistrationSearch::InitFromAddresses(
    uintptr_t codeRegistration,
    uintptr_t metadataRegistration,
    RegistrationInitResult& out,
    std::string& error)
{
    out = {};
    if (metadataRegistration == 0)
    {
        error = "MetadataRegistration address is zero";
        return false;
    }
    if (codeRegistration == 0)
    {
        error = "CodeRegistration address is zero";
        return false;
    }
    DumpSdkLog(DumpSdkLogLevel::Info,
        std::format("[Il2CppOffline] RegistrationSearch: InitFromAddresses cr=0x{:X} mr=0x{:X}",
            codeRegistration,
            metadataRegistration));
    if (!AutoPlusInit(codeRegistration, metadataRegistration, out))
    {
        error = "AutoPlusInit failed for provided registration addresses";
        return false;
    }
    out.success = true;
    return true;
}

uintptr_t RegistrationSearch::FindCodeRegistration()
{
    if (input_.version >= 24.2)
    {
        uintptr_t codeRegistration = FindCodeRegistrationData();
        if (codeRegistration == 0)
        {
            codeRegistration = FindCodeRegistrationExec();
            if (codeRegistration != 0)
            {
                pointerInExec_ = true;
            }
        }
        if (codeRegistration == 0)
        {
            codeRegistration = FindCodeRegistrationByCodeGenModules();
        }
        return codeRegistration;
    }
    return FindCodeRegistrationOld();
}

uintptr_t RegistrationSearch::FindMetadataRegistration()
{
    if (input_.version < 19.0)
    {
        return 0;
    }
    if (input_.version >= 27.0)
    {
        uintptr_t metadataRegistration = FindMetadataRegistrationV21();
        if (metadataRegistration == 0)
        {
            metadataRegistration = FindMetadataRegistrationByMetadataUsages();
        }
        return metadataRegistration;
    }
    uintptr_t metadataRegistration = FindMetadataRegistrationOld();
    if (metadataRegistration == 0)
    {
        metadataRegistration = FindMetadataRegistrationByMetadataUsages();
    }
    return metadataRegistration;
}

} // namespace er2
