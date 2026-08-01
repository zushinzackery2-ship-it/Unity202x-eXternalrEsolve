#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <algorithm>
#include <cstring>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

uintptr_t RegistrationSearch::FindCodeRegistrationByCodeGenModules()
{
    if (input_.imageCount <= 0)
    {
        return 0;
    }
    for (const PeSearchSection& sec : BuildDataSearchSections(pe_))
    {
        const size_t secLen = sec.offsetEnd - sec.offset;
        if (secLen < kPtrSize * 2 || secLen > 0x7FFFFFFF)
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
        if (secLen < kPtrSize * 2 || secLen > 0x7FFFFFFF)
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

uintptr_t RegistrationSearch::FindBestCodeRegistrationStartFromHit(
    uintptr_t codeGenModulesCountAddress,
    uint64_t codeGenModules)
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

uintptr_t RegistrationSearch::FindBestMetadataRegistrationStartFromHit(
    uintptr_t metadataUsagesCountAddress,
    uint64_t metadataUsages)
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
        if (mr.metadataUsagesCount != static_cast<uint64_t>(input_.metadataUsagesCount) ||
            mr.metadataUsages != metadataUsages)
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
        const int sample = (std::min)(input_.imageCount, 3);
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

} // namespace er2
