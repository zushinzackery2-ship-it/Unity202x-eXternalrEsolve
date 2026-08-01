#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <array>
#include <cstring>

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

uintptr_t RegistrationSearch::FindCodeRegistrationData()
{
    return FindCodeRegistration2019(BuildDataSearchSections(pe_));
}

uintptr_t RegistrationSearch::FindCodeRegistrationExec()
{
    return FindCodeRegistration2019(BuildExecSearchSections(pe_));
}

uintptr_t RegistrationSearch::FindCodeRegistration2019(const std::vector<PeSearchSection>& sections)
{
    for (const PeSearchSection& section : sections)
    {
        const size_t sectionLength = section.offsetEnd - section.offset;
        if (sectionLength == 0 || sectionLength > 0x7FFFFFFF)
        {
            continue;
        }

        std::vector<uint8_t> bytes;
        try
        {
            bytes = pe_.ReadBytes(section.address, sectionLength);
        }
        catch (...)
        {
            continue;
        }

        const std::vector<size_t> hits = SearchBytes(
            bytes.data(),
            bytes.size(),
            kMscorlibFeature.data(),
            kMscorlibFeature.size());
        for (const size_t index : hits)
        {
            const uintptr_t moduleName = static_cast<uintptr_t>(index) + section.address;
            for (const uintptr_t firstReference : FindReference(moduleName))
            {
                for (const uintptr_t secondReference : FindReference(firstReference))
                {
                    const uintptr_t registration = FindCodeRegistrationFromSecondReference(secondReference);
                    if (registration != 0)
                    {
                        return registration;
                    }
                }
            }
        }
    }
    return 0;
}

uintptr_t RegistrationSearch::FindCodeRegistrationFromSecondReference(uintptr_t reference)
{
    if (input_.version >= 27.0)
    {
        for (int imageIndex = input_.imageCount - 1; imageIndex >= 0; --imageIndex)
        {
            const uintptr_t imageModule = reference - static_cast<uintptr_t>(imageIndex) * kPtrSize;
            for (const uintptr_t thirdReference : FindReference(imageModule))
            {
                if (ReadIntPtrAbs(pe_, thirdReference - kPtrSize) != input_.imageCount)
                {
                    continue;
                }
                const size_t pointerCount = input_.version >= 29.0 ? 14 : 13;
                return thirdReference - kPtrSize * pointerCount;
            }
        }
        return 0;
    }

    for (int imageIndex = 0; imageIndex < input_.imageCount; ++imageIndex)
    {
        const uintptr_t imageModule = reference - static_cast<uintptr_t>(imageIndex) * kPtrSize;
        const std::vector<uintptr_t> references = FindReference(imageModule);
        if (!references.empty())
        {
            return references.front() - kPtrSize * 13;
        }
    }
    return 0;
}

} // namespace er2
