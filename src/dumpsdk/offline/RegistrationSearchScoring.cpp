#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <cstdlib>

namespace er2
{

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

} // namespace er2
