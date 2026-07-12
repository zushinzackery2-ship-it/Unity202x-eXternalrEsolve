#include <er2/unity2/dumpsdk/offline/Il2CppStructs.h>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>
#include <er2/unity2/dumpsdk/offline/PeImage.h>

#include <cstring>

namespace er2
{

void Il2CppTypeRuntime::Init(double version)
{
    attrs = bits & 0xFFFFu;
    type = static_cast<Il2CppTypeEnum>((bits >> 16) & 0xFFu);
    if (version >= 27.2)
    {
        num_mods = (bits >> 24) & 0x1Fu;
        byref = (bits >> 29) & 1u;
        pinned = (bits >> 30) & 1u;
        valuetype = bits >> 31;
    }
    else
    {
        num_mods = (bits >> 24) & 0x3Fu;
        byref = (bits >> 30) & 1u;
        pinned = bits >> 31;
    }
}

namespace
{

constexpr size_t kPtrSize = 8;

bool ReadU64(const PeImage& pe, uintptr_t va, uint64_t& out)
{
    return TryReadU64(pe, va, out);
}

bool SkipU64(const PeImage& pe, uintptr_t& va)
{
    uint64_t dummy = 0;
    if (!ReadU64(pe, va, dummy))
    {
        return false;
    }
    va += kPtrSize;
    return true;
}

bool ReadFieldIf(const PeImage& pe, uintptr_t& va, bool condition, uint64_t& field)
{
    if (!condition)
    {
        return true;
    }
    if (!ReadU64(pe, va, field))
    {
        return false;
    }
    va += kPtrSize;
    return true;
}

bool BetweenVersion(double version, double minVersion, double maxVersion)
{
    return version + 1e-6 >= minVersion && version <= maxVersion + 1e-6;
}

} // namespace

bool ReadCodeRegistration(const PeImage& pe, uintptr_t va, double version, CodeRegistrationView& out)
{
    out = {};
    uintptr_t cursor = va;

    if (!ReadFieldIf(pe, cursor, version <= 24.1, out.methodPointersCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version <= 24.1, out.methodPointers))
    {
        return false;
    }
    if (version <= 21.0)
    {
        uint64_t skipA = 0;
        uint64_t skipB = 0;
        if (!ReadFieldIf(pe, cursor, true, skipA) || !ReadFieldIf(pe, cursor, true, skipB))
        {
            return false;
        }
    }
    if (!ReadFieldIf(pe, cursor, version >= 22.0, out.reversePInvokeWrapperCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 22.0, out.reversePInvokeWrappers))
    {
        return false;
    }
    if (version <= 22.0)
    {
        uint64_t skipFields[6] = {};
        for (size_t i = 0; i < 6; ++i)
        {
            if (!ReadFieldIf(pe, cursor, true, skipFields[i]))
            {
                return false;
            }
        }
    }
    if (!ReadU64(pe, cursor, out.genericMethodPointersCount))
    {
        return false;
    }
    cursor += kPtrSize;
    if (!ReadU64(pe, cursor, out.genericMethodPointers))
    {
        return false;
    }
    cursor += kPtrSize;
    if (BetweenVersion(version, 24.5, 24.5) || version >= 27.1)
    {
        if (!ReadU64(pe, cursor, out.genericAdjustorThunks))
        {
            return false;
        }
        cursor += kPtrSize;
    }
    if (!ReadU64(pe, cursor, out.invokerPointersCount))
    {
        return false;
    }
    cursor += kPtrSize;
    if (!ReadU64(pe, cursor, out.invokerPointers))
    {
        return false;
    }
    cursor += kPtrSize;
    if (!ReadFieldIf(pe, cursor, version <= 24.5, out.customAttributeCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version <= 24.5, out.customAttributeGenerators))
    {
        return false;
    }
    if (BetweenVersion(version, 21.0, 22.0))
    {
        uint64_t skipA = 0;
        uint64_t skipB = 0;
        if (!ReadFieldIf(pe, cursor, true, skipA) || !ReadFieldIf(pe, cursor, true, skipB))
        {
            return false;
        }
    }
    if (!ReadFieldIf(pe, cursor, version >= 22.0, out.unresolvedVirtualCallCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 22.0, out.unresolvedVirtualCallPointers))
    {
        return false;
    }
    if (BetweenVersion(version, 29.1, 30.99))
    {
        if (!ReadU64(pe, cursor, out.unresolvedInstanceCallPointers))
        {
            return false;
        }
        cursor += kPtrSize;
        if (!ReadU64(pe, cursor, out.unresolvedStaticCallPointers))
        {
            return false;
        }
        cursor += kPtrSize;
    }
    if (!ReadFieldIf(pe, cursor, version >= 23.0, out.interopDataCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 23.0, out.interopData))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 24.3, out.windowsRuntimeFactoryCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 24.3, out.windowsRuntimeFactoryTable))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 24.2, out.codeGenModulesCount))
    {
        return false;
    }
    if (!ReadFieldIf(pe, cursor, version >= 24.2, out.codeGenModules))
    {
        return false;
    }
    return true;
}

bool ReadMetadataRegistration(const PeImage& pe, uintptr_t va, double version, MetadataRegistrationView& out)
{
    out = {};
    try
    {
        out.genericClassesCount = static_cast<int64_t>(pe.ReadU64(va));
        out.genericClasses = pe.ReadU64(va + kPtrSize);
        out.genericInstsCount = static_cast<int64_t>(pe.ReadU64(va + kPtrSize * 2));
        out.genericInsts = pe.ReadU64(va + kPtrSize * 3);
        out.genericMethodTableCount = static_cast<int64_t>(pe.ReadU64(va + kPtrSize * 4));
        out.genericMethodTable = pe.ReadU64(va + kPtrSize * 5);
        out.typesCount = static_cast<int64_t>(pe.ReadU64(va + kPtrSize * 6));
        out.types = pe.ReadU64(va + kPtrSize * 7);
        out.methodSpecsCount = static_cast<int64_t>(pe.ReadU64(va + kPtrSize * 8));
        out.methodSpecs = pe.ReadU64(va + kPtrSize * 9);
    }
    catch (...)
    {
        return false;
    }
    if (version <= 16.0)
    {
        pe.ReadU64(va + kPtrSize * 10);
        pe.ReadU64(va + kPtrSize * 11);
    }
    const size_t fieldOffsetsIndex = version <= 16.0 ? 12 : 10;
    try
    {
        out.fieldOffsetsCount = static_cast<int64_t>(pe.ReadU64(va + kPtrSize * fieldOffsetsIndex));
        out.fieldOffsets = pe.ReadU64(va + kPtrSize * (fieldOffsetsIndex + 1));
        out.typeDefinitionsSizesCount = static_cast<int64_t>(pe.ReadU64(va + kPtrSize * (fieldOffsetsIndex + 2)));
        out.typeDefinitionsSizes = pe.ReadU64(va + kPtrSize * (fieldOffsetsIndex + 3));
        if (version >= 19.0)
        {
            out.metadataUsagesCount = pe.ReadU64(va + kPtrSize * (fieldOffsetsIndex + 4));
            out.metadataUsages = pe.ReadU64(va + kPtrSize * (fieldOffsetsIndex + 5));
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool ReadCodeGenModule(const PeImage& pe, uintptr_t va, double version, CodeGenModuleView& out)
{
    out = {};
    if (!ReadU64(pe, va, out.moduleName))
    {
        return false;
    }
    uintptr_t cursor = va + kPtrSize;
    try
    {
        out.methodPointerCount = static_cast<int64_t>(pe.ReadU64(cursor));
    }
    catch (...)
    {
        return false;
    }
    cursor += kPtrSize;
    if (!ReadU64(pe, cursor, out.methodPointers))
    {
        return false;
    }
    cursor += kPtrSize;
    if (BetweenVersion(version, 24.5, 24.5) || version >= 27.1)
    {
        try
        {
            out.adjustorThunkCount = static_cast<int64_t>(pe.ReadU64(cursor));
        }
        catch (...)
        {
            return false;
        }
        cursor += kPtrSize;
        if (!ReadU64(pe, cursor, out.adjustorThunks))
        {
            return false;
        }
        cursor += kPtrSize;
    }
    if (!ReadU64(pe, cursor, out.invokerIndices))
    {
        return false;
    }
    return true;
}

bool ReadIl2CppType(const PeImage& pe, uintptr_t va, double version, Il2CppTypeRuntime& out)
{
    out = {};
    try
    {
        out.datapoint = pe.ReadU64(va);
        out.bits = static_cast<uint32_t>(pe.ReadU64(va + kPtrSize));
    }
    catch (...)
    {
        return false;
    }
    out.Init(version);
    return true;
}

} // namespace er2
