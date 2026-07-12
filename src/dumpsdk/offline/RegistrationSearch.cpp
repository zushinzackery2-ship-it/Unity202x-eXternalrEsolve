#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/dump_log.hpp>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;
constexpr std::array<uint8_t, 13> kMscorlibFeature = {
    0x6D, 0x73, 0x63, 0x6F, 0x72, 0x6C, 0x69, 0x62, 0x2E, 0x64, 0x6C, 0x6C, 0x00
};

std::vector<size_t> SearchBytes(const uint8_t* data, size_t size, const uint8_t* pattern, size_t patternSize)
{
    std::vector<size_t> hits;
    if (patternSize == 0 || size < patternSize)
    {
        return hits;
    }
    const size_t limit = size - patternSize;
    for (size_t i = 0; i <= limit; ++i)
    {
        if (std::memcmp(data + i, pattern, patternSize) == 0)
        {
            hits.push_back(i);
        }
    }
    return hits;
}

} // namespace

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

bool RegistrationSearch::InitFromAddresses(uintptr_t codeRegistration,
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
        uintptr_t mr = FindMetadataRegistrationV21();
        if (mr == 0)
        {
            mr = FindMetadataRegistrationByMetadataUsages();
        }
        return mr;
    }
    uintptr_t mrOld = FindMetadataRegistrationOld();
    if (mrOld == 0)
    {
        mrOld = FindMetadataRegistrationByMetadataUsages();
    }
    return mrOld;
}

uintptr_t RegistrationSearch::FindCodeRegistrationOld()
{
    for (const PeSearchSection& section : BuildDataSearchSections(pe_))
    {
        uint64_t abs = section.address;
        const uint64_t end = section.addressEnd - kPtrSize;
        while (abs < end)
        {
            if (ReadIntPtrAbs(pe_, abs) == input_.methodCount)
            {
                try
                {
                    const uint64_t pointerVa = ReadUIntPtrAbs(pe_, abs + kPtrSize);
                    const uint64_t pointerRa = pe_.MapVATR(pointerVa);
                    if (pointerRa != 0 && IsOffsetInData(pe_, pointerRa))
                    {
                        const size_t sample = static_cast<size_t>(std::min<int64_t>(input_.methodCount, 3));
                        bool valid = sample > 0;
                        for (size_t i = 0; i < sample && valid; ++i)
                        {
                            const uint64_t ptr = ReadUIntPtrAbs(pe_, pointerVa + i * kPtrSize);
                            if (ptr == 0 || !IsPointerInExec(pe_, ptr))
                            {
                                valid = false;
                            }
                        }
                        if (valid)
                        {
                            return static_cast<uintptr_t>(abs);
                        }
                    }
                }
                catch (...)
                {
                }
            }
            abs += kPtrSize;
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindMetadataRegistrationOld()
{
    for (const PeSearchSection& section : BuildDataSearchSections(pe_))
    {
        uint64_t abs = section.address;
        const uint64_t end = section.addressEnd - kPtrSize;
        while (abs < end)
        {
            if (ReadIntPtrAbs(pe_, abs) == input_.typeDefCount)
            {
                try
                {
                    const uint64_t pointerVa = ReadUIntPtrAbs(pe_, abs + kPtrSize * 3);
                    const uint64_t pointerRa = pe_.MapVATR(pointerVa);
                    if (pointerRa != 0 && IsOffsetInData(pe_, pointerRa))
                    {
                        const size_t sample = static_cast<size_t>(std::min<int64_t>(input_.metadataUsagesCount, 3));
                        bool valid = input_.metadataUsagesCount > 0;
                        for (size_t i = 0; i < sample && valid; ++i)
                        {
                            const uint64_t ptr = ReadUIntPtrAbs(pe_, pointerVa + i * kPtrSize);
                            if (ptr == 0 || !IsPointerInBss(pe_, ptr))
                            {
                                valid = false;
                            }
                        }
                        if (valid)
                        {
                            return static_cast<uintptr_t>(abs - kPtrSize * 12);
                        }
                    }
                }
                catch (...)
                {
                }
            }
            abs += kPtrSize;
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindMetadataRegistrationV21()
{
    // Sidecar pattern: fieldOffsetsCount, [skip fieldOffsets], typeDefinitionsSizesCount.
    // Validate via MetadataRegistration layout instead of treating typeDefinitionsSizes as a pointer table.
    for (const PeSearchSection& section : BuildDataSearchSections(pe_))
    {
        uint64_t abs = section.address;
        const uint64_t end = section.addressEnd - kPtrSize;
        while (abs < end)
        {
            if (ReadIntPtrAbs(pe_, abs) == input_.typeDefCount &&
                ReadIntPtrAbs(pe_, abs + kPtrSize * 2) == input_.typeDefCount)
            {
                const uintptr_t candidate = static_cast<uintptr_t>(abs - kPtrSize * 10);
                MetadataRegistrationView mr{};
                if (!ReadMetadataRegistration(pe_, candidate, input_.version, mr))
                {
                    abs += kPtrSize;
                    continue;
                }
                if (mr.fieldOffsetsCount != input_.typeDefCount ||
                    mr.typeDefinitionsSizesCount != input_.typeDefCount)
                {
                    abs += kPtrSize;
                    continue;
                }
                if (mr.typesCount <= 0 || mr.types == 0 || mr.fieldOffsets == 0 || mr.typeDefinitionsSizes == 0)
                {
                    abs += kPtrSize;
                    continue;
                }
                if (pe_.MapVATR(mr.types) == 0 ||
                    pe_.MapVATR(mr.fieldOffsets) == 0 ||
                    pe_.MapVATR(mr.typeDefinitionsSizes) == 0)
                {
                    abs += kPtrSize;
                    continue;
                }

                bool typesOk = true;
                const size_t sample = static_cast<size_t>(std::min<int64_t>(mr.typesCount, 3));
                for (size_t i = 0; i < sample && typesOk; ++i)
                {
                    const uint64_t typePtr = ReadUIntPtrAbs(pe_, mr.types + i * kPtrSize);
                    if (typePtr == 0)
                    {
                        typesOk = false;
                        break;
                    }
                    if (pointerInExec_)
                    {
                        typesOk = IsPointerInExec(pe_, typePtr);
                    }
                    else
                    {
                        typesOk = IsPointerInData(pe_, typePtr);
                    }
                }
                if (typesOk)
                {
                    return candidate;
                }
            }
            abs += kPtrSize;
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindCodeRegistrationData()
{
    return FindCodeRegistration2019(BuildDataSearchSections(pe_));
}

uintptr_t RegistrationSearch::FindCodeRegistrationExec()
{
    return FindCodeRegistration2019(BuildExecSearchSections(pe_));
}

uintptr_t RegistrationSearch::FindCodeRegistration2019(const std::vector<PeSearchSection>& secs)
{
    for (const auto& sec : secs)
    {
        const size_t secLen = sec.offsetEnd - sec.offset;
        if (secLen == 0 || secLen > 0x7FFFFFFF)
        {
            continue;
        }
        std::vector<uint8_t> buff;
        try
        {
            buff = pe_.ReadBytes(sec.address, secLen);
        }
        catch (...)
        {
            continue;
        }
        const auto hits = SearchBytes(buff.data(), buff.size(), kMscorlibFeature.data(), kMscorlibFeature.size());
        for (const size_t index : hits)
        {
            const uintptr_t dllVa = static_cast<uintptr_t>(index) + sec.address;
            for (const uintptr_t refVa : FindReference(dllVa))
            {
                for (const uintptr_t refVa2 : FindReference(refVa))
                {
                    if (input_.version >= 27.0)
                    {
                        for (int i = input_.imageCount - 1; i >= 0; --i)
                        {
                            for (const uintptr_t refVa3 : FindReference(refVa2 - static_cast<uintptr_t>(i) * kPtrSize))
                            {
                                if (ReadIntPtrAbs(pe_, refVa3 - kPtrSize) == input_.imageCount)
                                {
                                    if (input_.version >= 29.0)
                                    {
                                        return refVa3 - kPtrSize * 14;
                                    }
                                    return refVa3 - kPtrSize * 13;
                                }
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; i < input_.imageCount; ++i)
                        {
                            for (const uintptr_t refVa3 : FindReference(refVa2 - static_cast<uintptr_t>(i) * kPtrSize))
                            {
                                return refVa3 - kPtrSize * 13;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindCodeRegistrationByCodeGenModules()
{
    if (input_.imageCount <= 0)
    {
        return 0;
    }
    for (const PeSearchSection& sec : BuildDataSearchSections(pe_))
    {
        const size_t secLen = sec.offsetEnd - sec.offset;
        if (secLen == 0 || secLen > 0x7FFFFFFF)
        {
            continue;
        }
        std::vector<uint8_t> buff;
        try
        {
            buff = pe_.ReadBytes(sec.address, secLen);
        }
        catch (...)
        {
            continue;
        }
        const size_t end = buff.size() - kPtrSize * 2;
        for (size_t index = 0; index <= end; index += kPtrSize)
        {
            uint64_t count = 0;
            std::memcpy(&count, buff.data() + index, sizeof(count));
            if (count != static_cast<uint64_t>(input_.imageCount))
            {
                continue;
            }
            uint64_t ptr = 0;
            std::memcpy(&ptr, buff.data() + index + kPtrSize, sizeof(ptr));
            if (ptr == 0 || !TryCheckCodeGenModulesArray(ptr))
            {
                continue;
            }
            const uintptr_t hit = static_cast<uintptr_t>(index) + sec.address;
            const uintptr_t best = FindBestCodeRegistrationStartFromHit(hit, ptr);
            if (best != 0)
            {
                return best;
            }
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindMetadataRegistrationByMetadataUsages()
{
    if (input_.metadataUsagesCount <= 0)
    {
        return 0;
    }
    for (const PeSearchSection& sec : BuildDataSearchSections(pe_))
    {
        const size_t secLen = sec.offsetEnd - sec.offset;
        if (secLen == 0 || secLen > 0x7FFFFFFF)
        {
            continue;
        }
        std::vector<uint8_t> buff;
        try
        {
            buff = pe_.ReadBytes(sec.address, secLen);
        }
        catch (...)
        {
            continue;
        }
        const size_t end = buff.size() - kPtrSize * 2;
        for (size_t index = 0; index <= end; index += kPtrSize)
        {
            uint64_t count = 0;
            std::memcpy(&count, buff.data() + index, sizeof(count));
            if (count != static_cast<uint64_t>(input_.metadataUsagesCount))
            {
                continue;
            }
            uint64_t ptr = 0;
            std::memcpy(&ptr, buff.data() + index + kPtrSize, sizeof(ptr));
            if (ptr == 0 || !TryCheckPointerArrayInBss(ptr, static_cast<uint64_t>(input_.metadataUsagesCount)))
            {
                continue;
            }
            const uintptr_t hit = static_cast<uintptr_t>(index) + sec.address;
            const uintptr_t best = FindBestMetadataRegistrationStartFromHit(hit, ptr);
            if (best != 0)
            {
                return best;
            }
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindBestCodeRegistrationStartFromHit(uintptr_t codeGenModulesCountAddress, uint64_t codeGenModules)
{
    int bestScore = -1;
    uintptr_t bestAddr = 0;
    for (int back = 0; back <= 64; ++back)
    {
        const uintptr_t start = codeGenModulesCountAddress - static_cast<uintptr_t>(back) * kPtrSize;
        CodeRegistrationView cr{};
        if (!ReadCodeRegistration(pe_, start, input_.version, cr))
        {
            continue;
        }
        if (cr.codeGenModulesCount != static_cast<uint64_t>(input_.imageCount) || cr.codeGenModules != codeGenModules)
        {
            continue;
        }
        int score = 0;
        if (cr.invokerPointersCount > 0 && cr.invokerPointers != 0)
        {
            score += 2;
            if (TryCheckPointerArrayInExec(cr.invokerPointers, cr.invokerPointersCount))
            {
                score += 6;
            }
        }
        if (cr.genericMethodPointersCount > 0 && cr.genericMethodPointers != 0)
        {
            score += 2;
            if (TryCheckPointerArrayInExec(cr.genericMethodPointers, cr.genericMethodPointersCount))
            {
                score += 6;
            }
        }
        if (cr.reversePInvokeWrapperCount == 0 || cr.reversePInvokeWrappers != 0)
        {
            score += 1;
        }
        if (cr.unresolvedVirtualCallCount == 0 || cr.unresolvedVirtualCallPointers != 0)
        {
            score += 1;
        }
        if (score > bestScore)
        {
            bestScore = score;
            bestAddr = start;
        }
    }
    return bestAddr;
}

uintptr_t RegistrationSearch::FindBestMetadataRegistrationStartFromHit(uintptr_t metadataUsagesCountAddress, uint64_t metadataUsages)
{
    int bestScore = -1;
    uintptr_t bestAddr = 0;
    for (int back = 0; back <= 64; ++back)
    {
        const uintptr_t start = metadataUsagesCountAddress - static_cast<uintptr_t>(back) * kPtrSize;
        MetadataRegistrationView mr{};
        if (!ReadMetadataRegistration(pe_, start, input_.version, mr))
        {
            continue;
        }
        if (mr.metadataUsagesCount != static_cast<uint64_t>(input_.metadataUsagesCount) || mr.metadataUsages != metadataUsages)
        {
            continue;
        }
        int score = 0;
        if (mr.typesCount > 0 && mr.types != 0)
        {
            score += 1;
        }
        if (mr.fieldOffsetsCount > 0 && mr.fieldOffsets != 0)
        {
            score += 1;
        }
        if (TryCheckPointerArrayInBss(mr.metadataUsages, static_cast<uint64_t>(input_.metadataUsagesCount)))
        {
            score += 6;
        }
        if (score > bestScore)
        {
            bestScore = score;
            bestAddr = start;
        }
    }
    return bestAddr;
}

bool RegistrationSearch::TryCheckCodeGenModulesArray(uint64_t codeGenModules)
{
    try
    {
        const uint64_t ra = pe_.MapVATR(codeGenModules);
        if (ra == 0)
        {
            return false;
        }
        const int sample = std::min(input_.imageCount, 3);
        for (int i = 0; i < sample; ++i)
        {
            const uint64_t modulePtr = ReadUIntPtrAbs(pe_, codeGenModules + i * kPtrSize);
            if (modulePtr == 0)
            {
                return false;
            }
            CodeGenModuleView module{};
            if (!ReadCodeGenModule(pe_, static_cast<uintptr_t>(modulePtr), input_.version, module))
            {
                return false;
            }
            const std::string name = pe_.ReadCString(module.moduleName);
            if (name.empty() || name.find(".dll") == std::string::npos)
            {
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RegistrationSearch::TryCheckPointerArrayInExec(uint64_t arrayPtr, uint64_t count)
{
    try
    {
        if (arrayPtr == 0 || count == 0)
        {
            return false;
        }
        const uint64_t ra = pe_.MapVATR(arrayPtr);
        if (ra == 0)
        {
            return false;
        }
        const int sample = static_cast<int>(std::min<uint64_t>(3, count));
        for (int i = 0; i < sample; ++i)
        {
            const uint64_t ptr = ReadUIntPtrAbs(pe_, arrayPtr + i * kPtrSize);
            if (ptr == 0 || !IsPointerInExec(pe_, ptr))
            {
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RegistrationSearch::TryCheckPointerArrayInData(uint64_t arrayPtr, uint64_t count)
{
    try
    {
        if (arrayPtr == 0 || count == 0)
        {
            return false;
        }
        const int sample = static_cast<int>(std::min<uint64_t>(3, count));
        for (int i = 0; i < sample; ++i)
        {
            const uint64_t ptr = ReadUIntPtrAbs(pe_, arrayPtr + i * kPtrSize);
            if (ptr == 0 || !IsPointerInData(pe_, ptr))
            {
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RegistrationSearch::TryCheckPointerArrayInBss(uint64_t arrayPtr, uint64_t count)
{
    try
    {
        if (arrayPtr == 0 || count == 0)
        {
            return false;
        }
        const uint64_t ra = pe_.MapVATR(arrayPtr);
        if (ra == 0)
        {
            return false;
        }
        const int sample = static_cast<int>(std::min<uint64_t>(3, count));
        for (int i = 0; i < sample; ++i)
        {
            const uint64_t ptr = ReadUIntPtrAbs(pe_, arrayPtr + i * kPtrSize);
            if (ptr == 0 || !IsPointerInBss(pe_, ptr))
            {
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::vector<uintptr_t> RegistrationSearch::FindReference(uintptr_t addr)
{
    std::vector<uintptr_t> refs;
    for (const PeSearchSection& dataSec : BuildDataSearchSections(pe_))
    {
        uint64_t abs = dataSec.address;
        const uint64_t end = dataSec.addressEnd - kPtrSize;
        while (abs < end)
        {
            if (ReadUIntPtrAbs(pe_, abs) == addr)
            {
                refs.push_back(static_cast<uintptr_t>(abs));
            }
            abs += kPtrSize;
        }
    }
    return refs;
}

bool RegistrationSearch::IsMappable(uintptr_t addr)
{
    try
    {
        return pe_.MapVATR(addr) != 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RegistrationSearch::TryReadPointerAt(uintptr_t absAddr, uint64_t& value)
{
    value = 0;
    if (!TryReadU64(pe_, absAddr, value))
    {
        return false;
    }
    return value != 0 && IsMappable(static_cast<uintptr_t>(value));
}

int RegistrationSearch::ScoreCodeRegistrationCandidate(uintptr_t codeRegistration)
{
    try
    {
        CodeRegistrationView reg{};
        if (!ReadCodeRegistration(pe_, codeRegistration, input_.version, reg))
        {
            return -1;
        }
        if (input_.version >= 24.2)
        {
            if (reg.codeGenModules == 0 || reg.codeGenModulesCount == 0)
            {
                return -1;
            }
            if (input_.imageCount > 0)
            {
                const uint64_t expected = static_cast<uint64_t>(input_.imageCount);
                const uint64_t minCount = expected > 1 ? expected / 2 : expected;
                const uint64_t maxCount = expected * 2;
                if (reg.codeGenModulesCount < minCount || reg.codeGenModulesCount > maxCount)
                {
                    return -1;
                }
            }
        }
        int score = 0;
        if (input_.imageCount > 0)
        {
            const uint64_t expected = static_cast<uint64_t>(input_.imageCount);
            score += reg.codeGenModulesCount == expected ? 1000 : 200;
        }
        if (reg.codeGenModules != 0 && IsMappable(static_cast<uintptr_t>(reg.codeGenModules)))
        {
            score += 200;
        }
        if (input_.methodCount > 0)
        {
            const uint64_t expected = static_cast<uint64_t>(input_.methodCount);
            const uint64_t gmpCount = reg.genericMethodPointersCount;
            if (gmpCount > 0 && gmpCount <= expected * 200 && gmpCount <= 5000000)
            {
                score += 400;
            }
            else if (gmpCount == 0)
            {
                score += 10;
            }
            else
            {
                score -= 200;
            }
        }
        if (reg.genericMethodPointers != 0 && IsMappable(static_cast<uintptr_t>(reg.genericMethodPointers)))
        {
            score += 50;
        }
        if (reg.invokerPointers != 0 && IsMappable(static_cast<uintptr_t>(reg.invokerPointers)))
        {
            score += 50;
        }
        return score;
    }
    catch (...)
    {
        return -1;
    }
}

int RegistrationSearch::ScoreMetadataRegistrationCandidate(uintptr_t metadataRegistration)
{
    try
    {
        MetadataRegistrationView reg{};
        if (!ReadMetadataRegistration(pe_, metadataRegistration, input_.version, reg))
        {
            return -1;
        }
        if (reg.types == 0 || reg.typesCount <= 0)
        {
            return -1;
        }
        int score = 0;
        if (input_.typeDefCount > 0)
        {
            const int64_t expected = input_.typeDefCount;
            const int64_t diff = std::llabs(reg.typesCount - expected);
            if (diff == 0)
            {
                score += 1000;
            }
            else if (diff < expected / 10)
            {
                score += 300;
            }
            else
            {
                score -= 100;
            }
        }
        if (IsMappable(static_cast<uintptr_t>(reg.types)))
        {
            score += 200;
        }
        if (reg.methodSpecs != 0 && IsMappable(static_cast<uintptr_t>(reg.methodSpecs)))
        {
            score += 50;
        }
        if (reg.fieldOffsets != 0 && IsMappable(static_cast<uintptr_t>(reg.fieldOffsets)))
        {
            score += 50;
        }
        return score;
    }
    catch (...)
    {
        return -1;
    }
}

bool RegistrationSearch::AutoPlusInit(uintptr_t codeRegistration, uintptr_t metadataRegistration, RegistrationInitResult& out)
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

    struct ScoredMr { uintptr_t mr; int score; };
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

    struct ScoredCr { uintptr_t cr; int score; };
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

    for (const auto& mrItem : scoredMrs)
    {
        DumpSdkLog(DumpSdkLogLevel::Info,
            std::format("[Il2CppOffline] MetadataRegistration candidate: 0x{:X} score={}", mrItem.mr, mrItem.score));
        for (const auto& crItem : scoredCrs)
        {
            DumpSdkLog(DumpSdkLogLevel::Info,
                std::format("[Il2CppOffline] CodeRegistration candidate: 0x{:X} score={}", crItem.cr, crItem.score));
            if (TryInit(crItem.cr, mrItem.mr, out))
            {
                return true;
            }
        }
    }
    return false;
}

bool RegistrationSearch::TryInit(uintptr_t codeRegistration, uintptr_t metadataRegistration, RegistrationInitResult& out)
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
            std::vector<uintptr_t> pointers;
            pointers.resize(static_cast<size_t>(module.methodPointerCount));
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
