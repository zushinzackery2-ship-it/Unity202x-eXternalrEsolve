#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <algorithm>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

uintptr_t RegistrationSearch::FindCodeRegistrationOld()
{
    for (const PeSearchSection& section : BuildDataSearchSections(pe_))
    {
        if (section.addressEnd <= section.address + kPtrSize)
        {
            continue;
        }
        uint64_t address = section.address;
        const uint64_t end = section.addressEnd - kPtrSize;
        while (address < end)
        {
            if (ReadIntPtrAbs(pe_, address) == input_.methodCount &&
                TryValidateOldCodeRegistrationCandidate(static_cast<uintptr_t>(address)))
            {
                return static_cast<uintptr_t>(address);
            }
            address += kPtrSize;
        }
    }
    return 0;
}

bool RegistrationSearch::TryValidateOldCodeRegistrationCandidate(uintptr_t address)
{
    try
    {
        const uint64_t pointerVa = ReadUIntPtrAbs(pe_, address + kPtrSize);
        const uint64_t pointerRa = pe_.MapVATR(pointerVa);
        if (pointerRa == 0 || !IsOffsetInData(pe_, pointerRa))
        {
            return false;
        }
        const size_t sample = static_cast<size_t>(std::min<int64_t>(input_.methodCount, 3));
        if (sample == 0)
        {
            return false;
        }
        for (size_t i = 0; i < sample; ++i)
        {
            const uint64_t pointer = ReadUIntPtrAbs(pe_, pointerVa + i * kPtrSize);
            if (pointer == 0 || !IsPointerInExec(pe_, pointer))
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

uintptr_t RegistrationSearch::FindMetadataRegistrationOld()
{
    for (const PeSearchSection& section : BuildDataSearchSections(pe_))
    {
        if (section.addressEnd <= section.address + kPtrSize)
        {
            continue;
        }
        uint64_t address = section.address;
        const uint64_t end = section.addressEnd - kPtrSize;
        while (address < end)
        {
            if (ReadIntPtrAbs(pe_, address) == input_.typeDefCount &&
                TryValidateOldMetadataRegistrationCandidate(static_cast<uintptr_t>(address)))
            {
                return static_cast<uintptr_t>(address - kPtrSize * 12);
            }
            address += kPtrSize;
        }
    }
    return 0;
}

bool RegistrationSearch::TryValidateOldMetadataRegistrationCandidate(uintptr_t address)
{
    try
    {
        const uint64_t pointerVa = ReadUIntPtrAbs(pe_, address + kPtrSize * 3);
        const uint64_t pointerRa = pe_.MapVATR(pointerVa);
        if (pointerRa == 0 || !IsOffsetInData(pe_, pointerRa) || input_.metadataUsagesCount <= 0)
        {
            return false;
        }
        const size_t sample = static_cast<size_t>(std::min<int64_t>(input_.metadataUsagesCount, 3));
        for (size_t i = 0; i < sample; ++i)
        {
            const uint64_t pointer = ReadUIntPtrAbs(pe_, pointerVa + i * kPtrSize);
            if (pointer == 0 || !IsPointerInBss(pe_, pointer))
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

uintptr_t RegistrationSearch::FindMetadataRegistrationV21()
{
    for (const PeSearchSection& section : BuildDataSearchSections(pe_))
    {
        if (section.addressEnd <= section.address + kPtrSize)
        {
            continue;
        }
        uint64_t address = section.address;
        const uint64_t end = section.addressEnd - kPtrSize;
        while (address < end)
        {
            if (ReadIntPtrAbs(pe_, address) != input_.typeDefCount ||
                ReadIntPtrAbs(pe_, address + kPtrSize * 2) != input_.typeDefCount)
            {
                address += kPtrSize;
                continue;
            }

            const uintptr_t candidate = static_cast<uintptr_t>(address - kPtrSize * 10);
            MetadataRegistrationView registration{};
            if (!ReadMetadataRegistration(pe_, candidate, input_.version, registration) ||
                registration.fieldOffsetsCount != input_.typeDefCount ||
                registration.typeDefinitionsSizesCount != input_.typeDefCount ||
                registration.typesCount <= 0 ||
                registration.types == 0 ||
                registration.fieldOffsets == 0 ||
                registration.typeDefinitionsSizes == 0 ||
                pe_.MapVATR(registration.types) == 0 ||
                pe_.MapVATR(registration.fieldOffsets) == 0 ||
                pe_.MapVATR(registration.typeDefinitionsSizes) == 0)
            {
                address += kPtrSize;
                continue;
            }

            bool typesOk = true;
            const size_t sample = static_cast<size_t>(std::min<int64_t>(registration.typesCount, 3));
            for (size_t i = 0; i < sample; ++i)
            {
                const uint64_t typePointer = ReadUIntPtrAbs(pe_, registration.types + i * kPtrSize);
                if (typePointer == 0 ||
                    (pointerInExec_ && !IsPointerInExec(pe_, typePointer)) ||
                    (!pointerInExec_ && !IsPointerInData(pe_, typePointer)))
                {
                    typesOk = false;
                    break;
                }
            }
            if (typesOk)
            {
                return candidate;
            }
            address += kPtrSize;
        }
    }
    return 0;
}

} // namespace er2
