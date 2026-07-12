#pragma once

#include <er2/unity2/dumpsdk/offline/Il2CppStructs.h>
#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace er2
{

struct RegistrationSearchInput
{
    int methodCount = 0;
    int typeDefCount = 0;
    int imageCount = 0;
    int64_t metadataUsagesCount = 0;
    double version = 0;
};

struct RegistrationInitResult
{
    bool success = false;
    uintptr_t codeRegistrationVa = 0;
    uintptr_t metadataRegistrationVa = 0;
    double resolvedVersion = 0;
    bool pointerInExec = false;
    bool fieldOffsetsArePointers = false;
    std::vector<uint64_t> fieldOffsets;
    std::vector<uintptr_t> legacyMethodPointers;
    std::unordered_map<std::string, std::vector<uintptr_t>> codeGenModuleMethodPointers;
    std::vector<Il2CppTypeRuntime> types;
    std::unordered_map<uintptr_t, size_t> typePtrToIndex;
};

class RegistrationSearch
{
public:
    explicit RegistrationSearch(const PeImage& pe, const RegistrationSearchInput& input);

    bool FindAndInit(RegistrationInitResult& out, std::string& error);
    bool InitFromAddresses(uintptr_t codeRegistration,
        uintptr_t metadataRegistration,
        RegistrationInitResult& out,
        std::string& error);

private:
    const PeImage& pe_;
    RegistrationSearchInput input_;
    bool pointerInExec_ = false;

    uintptr_t FindCodeRegistration();
    uintptr_t FindMetadataRegistration();

    uintptr_t FindCodeRegistrationOld();
    uintptr_t FindMetadataRegistrationOld();
    uintptr_t FindMetadataRegistrationV21();
    uintptr_t FindCodeRegistrationData();
    uintptr_t FindCodeRegistrationExec();
    uintptr_t FindCodeRegistration2019(const std::vector<PeSearchSection>& secs);
    uintptr_t FindCodeRegistrationByCodeGenModules();
    uintptr_t FindMetadataRegistrationByMetadataUsages();

    uintptr_t FindBestCodeRegistrationStartFromHit(uintptr_t codeGenModulesCountAddress, uint64_t codeGenModules);
    uintptr_t FindBestMetadataRegistrationStartFromHit(uintptr_t metadataUsagesCountAddress, uint64_t metadataUsages);

    bool TryCheckCodeGenModulesArray(uint64_t codeGenModules);
    bool TryCheckPointerArrayInExec(uint64_t arrayPtr, uint64_t count);
    bool TryCheckPointerArrayInData(uint64_t arrayPtr, uint64_t count);
    bool TryCheckPointerArrayInBss(uint64_t arrayPtr, uint64_t count);

    std::vector<uintptr_t> FindReference(uintptr_t addr);

    bool AutoPlusInit(uintptr_t codeRegistration, uintptr_t metadataRegistration, RegistrationInitResult& out);
    bool TryInit(uintptr_t codeRegistration, uintptr_t metadataRegistration, RegistrationInitResult& out);

    int ScoreCodeRegistrationCandidate(uintptr_t codeRegistration);
    int ScoreMetadataRegistrationCandidate(uintptr_t metadataRegistration);

    bool IsMappable(uintptr_t addr);
    bool TryReadPointerAt(uintptr_t absAddr, uint64_t& value);
};

} // namespace er2
