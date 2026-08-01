#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <algorithm>
#include <cmath>
#include <format>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

bool RegistrationSearch::AutoPlusInit(
    uintptr_t codeRegistration,
    uintptr_t metadataRegistration,
    RegistrationInitResult& out)
{
    std::vector<uintptr_t> metadataCandidates;
    auto addMetadataCandidate = [&](uintptr_t value)
    {
        if (value == 0)
        {
            return;
        }
        if (std::find(metadataCandidates.begin(), metadataCandidates.end(), value) == metadataCandidates.end())
        {
            metadataCandidates.push_back(value);
        }
    };
    addMetadataCandidate(metadataRegistration);
    uint64_t derefMetadata = 0;
    if (TryReadPointerAt(metadataRegistration, derefMetadata))
    {
        addMetadataCandidate(static_cast<uintptr_t>(derefMetadata));
    }

    std::vector<uintptr_t> baseCodeRegistrations;
    auto addBaseCodeRegistration = [&](uintptr_t value)
    {
        if (value == 0)
        {
            return;
        }
        if (std::find(baseCodeRegistrations.begin(), baseCodeRegistrations.end(), value) == baseCodeRegistrations.end())
        {
            baseCodeRegistrations.push_back(value);
        }
    };
    addBaseCodeRegistration(codeRegistration);
    uint64_t derefCode = 0;
    if (TryReadPointerAt(codeRegistration, derefCode))
    {
        addBaseCodeRegistration(static_cast<uintptr_t>(derefCode));
    }

    std::vector<uintptr_t> candidates;
    auto addCandidate = [&](uintptr_t value)
    {
        if (value == 0)
        {
            return;
        }
        if (std::find(candidates.begin(), candidates.end(), value) == candidates.end())
        {
            candidates.push_back(value);
        }
    };
    const uintptr_t maxDelta = kPtrSize * 0x40;
    for (const uintptr_t baseCr : baseCodeRegistrations)
    {
        addCandidate(baseCr);
        for (uintptr_t delta = kPtrSize; delta <= maxDelta; delta += kPtrSize)
        {
            addCandidate(baseCr + delta);
            if (baseCr >= delta)
            {
                addCandidate(baseCr - delta);
            }
        }
    }

    struct ScoredMr
    {
        uintptr_t mr;
        int score;
    };
    std::vector<ScoredMr> scoredMrs;
    for (const uintptr_t mr : metadataCandidates)
    {
        const int score = ScoreMetadataRegistrationCandidate(mr);
        if (score >= 0)
        {
            scoredMrs.push_back({ mr, score });
        }
    }
    if (scoredMrs.empty())
    {
        for (const uintptr_t mr : metadataCandidates)
        {
            scoredMrs.push_back({ mr, 0 });
        }
    }
    std::sort(scoredMrs.begin(), scoredMrs.end(), [](const ScoredMr& a, const ScoredMr& b)
    {
        return a.score > b.score;
    });

    struct ScoredCr
    {
        uintptr_t cr;
        int score;
    };
    std::vector<ScoredCr> scoredCrs;
    for (const uintptr_t cr : candidates)
    {
        const int score = ScoreCodeRegistrationCandidate(cr);
        if (score >= 0)
        {
            scoredCrs.push_back({ cr, score });
        }
    }
    if (scoredCrs.empty())
    {
        for (const uintptr_t cr : candidates)
        {
            scoredCrs.push_back({ cr, 0 });
        }
    }
    std::sort(scoredCrs.begin(), scoredCrs.end(), [](const ScoredCr& a, const ScoredCr& b)
    {
        return a.score > b.score;
    });

    for (const ScoredMr& mrItem : scoredMrs)
    {
        DumpSdkLog(DumpSdkLogLevel::Info,
            std::format("[Il2CppOffline] MetadataRegistration candidate: 0x{:X} score={}",
                mrItem.mr,
                mrItem.score));
        for (const ScoredCr& crItem : scoredCrs)
        {
            DumpSdkLog(DumpSdkLogLevel::Info,
                std::format("[Il2CppOffline] CodeRegistration candidate: 0x{:X} score={}",
                    crItem.cr,
                    crItem.score));
            if (TryInit(crItem.cr, mrItem.mr, out))
            {
                return true;
            }
        }
    }
    return false;
}

bool RegistrationSearch::TryInit(
    uintptr_t codeRegistration,
    uintptr_t metadataRegistration,
    RegistrationInitResult& out)
{
    out = {};
    out.codeRegistrationVa = codeRegistration;
    out.metadataRegistrationVa = metadataRegistration;
    out.resolvedVersion = input_.version;
    out.pointerInExec = pointerInExec_;

    CodeRegistrationView codeReg{};
    if (!ReadCodeRegistration(pe_, codeRegistration, input_.version, codeReg))
    {
        return false;
    }

    MetadataRegistrationView metaReg{};
    if (!ReadMetadataRegistration(pe_, metadataRegistration, input_.version, metaReg))
    {
        return false;
    }

    if (input_.version >= 24.2)
    {
        if (codeReg.codeGenModulesCount == 0 || codeReg.codeGenModules == 0)
        {
            return false;
        }
        if (input_.imageCount > 0)
        {
            const uint64_t expected = static_cast<uint64_t>(input_.imageCount);
            const uint64_t minCount = expected > 1 ? expected / 2 : expected;
            const uint64_t maxCount = expected * 2;
            if (codeReg.codeGenModulesCount < minCount || codeReg.codeGenModulesCount > maxCount)
            {
                return false;
            }
        }
    }

    out.fieldOffsetsArePointers = input_.version > 21.0;
    if (std::fabs(input_.version - 21.0) < 1e-6)
    {
        uint32_t probe[6] = {};
        if (ReadAbs(pe_, metaReg.fieldOffsets, probe, sizeof(probe)))
        {
            out.fieldOffsetsArePointers =
                probe[0] == 0 && probe[1] == 0 && probe[2] == 0 &&
                probe[3] == 0 && probe[4] == 0 && probe[5] > 0;
        }
    }

    if (metaReg.fieldOffsets != 0 && metaReg.fieldOffsetsCount > 0)
    {
        const size_t count = static_cast<size_t>(metaReg.fieldOffsetsCount);
        out.fieldOffsets.resize(count);
        if (out.fieldOffsetsArePointers)
        {
            for (size_t i = 0; i < count; ++i)
            {
                out.fieldOffsets[i] = ReadUIntPtrAbs(pe_, metaReg.fieldOffsets + i * kPtrSize);
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                uint32_t value = 0;
                ReadAbs(pe_, metaReg.fieldOffsets + i * sizeof(uint32_t), &value, sizeof(value));
                out.fieldOffsets[i] = value;
            }
        }
    }

    if (metaReg.types != 0 && metaReg.typesCount > 0)
    {
        out.types.resize(static_cast<size_t>(metaReg.typesCount));
        for (size_t i = 0; i < out.types.size(); ++i)
        {
            const uint64_t typePtr = ReadUIntPtrAbs(pe_, metaReg.types + i * kPtrSize);
            if (typePtr == 0 || !ReadIl2CppType(pe_, static_cast<uintptr_t>(typePtr), input_.version, out.types[i]))
            {
                return false;
            }
            out.typePtrToIndex[static_cast<uintptr_t>(typePtr)] = i;
        }
    }

    if (input_.version >= 24.2)
    {
        for (uint64_t i = 0; i < codeReg.codeGenModulesCount; ++i)
        {
            const uint64_t modulePtr = ReadUIntPtrAbs(pe_, codeReg.codeGenModules + i * kPtrSize);
            if (modulePtr == 0)
            {
                continue;
            }
            CodeGenModuleView module{};
            if (!ReadCodeGenModule(pe_, static_cast<uintptr_t>(modulePtr), input_.version, module))
            {
                continue;
            }
            const std::string moduleName = pe_.ReadCString(module.moduleName);
            if (moduleName.empty())
            {
                continue;
            }
            std::vector<uintptr_t> pointers(static_cast<size_t>(module.methodPointerCount));
            for (int64_t j = 0; j < module.methodPointerCount; ++j)
            {
                pointers[static_cast<size_t>(j)] = static_cast<uintptr_t>(
                    ReadUIntPtrAbs(pe_, module.methodPointers + static_cast<uint64_t>(j) * kPtrSize));
            }
            out.codeGenModuleMethodPointers[moduleName] = std::move(pointers);
        }
    }
    else if (codeReg.methodPointers != 0 && codeReg.methodPointersCount > 0)
    {
        out.legacyMethodPointers.resize(static_cast<size_t>(codeReg.methodPointersCount));
        for (uint64_t i = 0; i < codeReg.methodPointersCount; ++i)
        {
            out.legacyMethodPointers[static_cast<size_t>(i)] = static_cast<uintptr_t>(
                ReadUIntPtrAbs(pe_, codeReg.methodPointers + i * kPtrSize));
        }
    }

    return true;
}

} // namespace er2
