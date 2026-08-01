#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <algorithm>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

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

} // namespace er2
