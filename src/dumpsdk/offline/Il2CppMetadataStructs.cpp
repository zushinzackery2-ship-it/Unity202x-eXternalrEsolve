#include <er2/unity2/dumpsdk/offline/Il2CppStructs.h>

#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

bool BetweenVersion(double version, double minVersion, double maxVersion)
{
    return version + 1e-6 >= minVersion && version <= maxVersion + 1e-6;
}

bool ReadSignedPointer(const PeImage& pe, uintptr_t& cursor, int64_t& target)
{
    uint64_t value = 0;
    if (!TryReadU64(pe, cursor, value))
    {
        return false;
    }
    target = static_cast<int64_t>(value);
    cursor += kPtrSize;
    return true;
}

bool ReadPointer(const PeImage& pe, uintptr_t& cursor, uint64_t& target)
{
    if (!TryReadU64(pe, cursor, target))
    {
        return false;
    }
    cursor += kPtrSize;
    return true;
}

bool SkipPointers(const PeImage& pe, uintptr_t& cursor, size_t count)
{
    uint64_t discard = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (!ReadPointer(pe, cursor, discard))
        {
            return false;
        }
    }
    return true;
}

} // namespace

bool ReadMetadataRegistration(
    const PeImage& pe,
    uintptr_t va,
    double version,
    MetadataRegistrationView& out)
{
    out = {};
    uintptr_t cursor = va;
    if (!ReadSignedPointer(pe, cursor, out.genericClassesCount) ||
        !ReadPointer(pe, cursor, out.genericClasses) ||
        !ReadSignedPointer(pe, cursor, out.genericInstsCount) ||
        !ReadPointer(pe, cursor, out.genericInsts) ||
        !ReadSignedPointer(pe, cursor, out.genericMethodTableCount) ||
        !ReadPointer(pe, cursor, out.genericMethodTable) ||
        !ReadSignedPointer(pe, cursor, out.typesCount) ||
        !ReadPointer(pe, cursor, out.types) ||
        !ReadSignedPointer(pe, cursor, out.methodSpecsCount) ||
        !ReadPointer(pe, cursor, out.methodSpecs))
    {
        return false;
    }
    if (version <= 16.0 && !SkipPointers(pe, cursor, 2))
    {
        return false;
    }
    if (!ReadSignedPointer(pe, cursor, out.fieldOffsetsCount) ||
        !ReadPointer(pe, cursor, out.fieldOffsets) ||
        !ReadSignedPointer(pe, cursor, out.typeDefinitionsSizesCount) ||
        !ReadPointer(pe, cursor, out.typeDefinitionsSizes))
    {
        return false;
    }
    if (version >= 19.0 &&
        (!ReadPointer(pe, cursor, out.metadataUsagesCount) ||
            !ReadPointer(pe, cursor, out.metadataUsages)))
    {
        return false;
    }
    return true;
}

bool ReadCodeGenModule(const PeImage& pe, uintptr_t va, double version, CodeGenModuleView& out)
{
    out = {};
    uintptr_t cursor = va;
    if (!ReadPointer(pe, cursor, out.moduleName) ||
        !ReadSignedPointer(pe, cursor, out.methodPointerCount) ||
        !ReadPointer(pe, cursor, out.methodPointers))
    {
        return false;
    }
    if (BetweenVersion(version, 24.5, 24.5) || version >= 27.1)
    {
        if (!ReadSignedPointer(pe, cursor, out.adjustorThunkCount) ||
            !ReadPointer(pe, cursor, out.adjustorThunks))
        {
            return false;
        }
    }
    if (!ReadPointer(pe, cursor, out.invokerIndices) ||
        !ReadPointer(pe, cursor, out.reversePInvokeWrapperCount) ||
        !ReadPointer(pe, cursor, out.reversePInvokeWrapperIndices) ||
        !ReadSignedPointer(pe, cursor, out.rgctxRangesCount) ||
        !ReadPointer(pe, cursor, out.rgctxRanges) ||
        !ReadSignedPointer(pe, cursor, out.rgctxsCount) ||
        !ReadPointer(pe, cursor, out.rgctxs) ||
        !ReadPointer(pe, cursor, out.debuggerMetadata))
    {
        return false;
    }
    if (BetweenVersion(version, 27.0, 27.2) &&
        !ReadPointer(pe, cursor, out.customAttributeCacheGenerator))
    {
        return false;
    }
    return true;
}

bool ReadIl2CppType(const PeImage& pe, uintptr_t va, double version, Il2CppTypeRuntime& out)
{
    out = {};
    if (!TryReadU64(pe, va, out.datapoint) ||
        !TryReadU32(pe, va + kPtrSize, out.bits))
    {
        return false;
    }
    out.Init(version);
    return true;
}

bool ReadIl2CppGenericClass(
    const PeImage& pe,
    uintptr_t va,
    double version,
    Il2CppGenericClass& out)
{
    out = {};
    uintptr_t cursor = va;
    if (version <= 24.5)
    {
        if (!ReadSignedPointer(pe, cursor, out.typeDefinitionIndex))
        {
            return false;
        }
    }
    else if (!ReadPointer(pe, cursor, out.type))
    {
        return false;
    }
    return ReadPointer(pe, cursor, out.context.class_inst) &&
        ReadPointer(pe, cursor, out.context.method_inst) &&
        ReadPointer(pe, cursor, out.cached_class);
}

bool ReadIl2CppArrayType(const PeImage& pe, uintptr_t va, Il2CppArrayType& out)
{
    out = {};
    uint32_t packed = 0;
    if (!TryReadU64(pe, va, out.etype) ||
        !TryReadU32(pe, va + kPtrSize, packed) ||
        !TryReadU64(pe, va + kPtrSize * 2, out.sizes) ||
        !TryReadU64(pe, va + kPtrSize * 3, out.lobounds))
    {
        return false;
    }
    out.rank = static_cast<uint8_t>(packed & 0xFFu);
    out.numsizes = static_cast<uint8_t>((packed >> 8) & 0xFFu);
    out.numlobounds = static_cast<uint8_t>((packed >> 16) & 0xFFu);
    return true;
}

bool ReadIl2CppGenericInst(const PeImage& pe, uintptr_t va, Il2CppGenericInst& out)
{
    out = {};
    uintptr_t cursor = va;
    return ReadSignedPointer(pe, cursor, out.type_argc) &&
        ReadPointer(pe, cursor, out.type_argv);
}

size_t MethodSpecSize()
{
    return sizeof(int32_t) * 3;
}

bool ReadIl2CppMethodSpec(const PeImage& pe, uintptr_t va, Il2CppMethodSpec& out)
{
    out = {};
    uint32_t value = 0;
    if (!TryReadU32(pe, va, value))
    {
        return false;
    }
    out.methodDefinitionIndex = static_cast<int32_t>(value);
    if (!TryReadU32(pe, va + 4, value))
    {
        return false;
    }
    out.classIndexIndex = static_cast<int32_t>(value);
    if (!TryReadU32(pe, va + 8, value))
    {
        return false;
    }
    out.methodIndexIndex = static_cast<int32_t>(value);
    return true;
}

size_t GenericMethodFunctionsSize(double version)
{
    return sizeof(int32_t) *
        ((BetweenVersion(version, 24.5, 24.5) || version >= 27.1) ? 4 : 3);
}

bool ReadGenericMethodFunctions(
    const PeImage& pe,
    uintptr_t va,
    double version,
    Il2CppGenericMethodFunctions& out)
{
    out = {};
    uint32_t value = 0;
    if (!TryReadU32(pe, va, value))
    {
        return false;
    }
    out.genericMethodIndex = static_cast<int32_t>(value);
    if (!TryReadU32(pe, va + 4, value))
    {
        return false;
    }
    out.indices.methodIndex = static_cast<int32_t>(value);
    if (!TryReadU32(pe, va + 8, value))
    {
        return false;
    }
    out.indices.invokerIndex = static_cast<int32_t>(value);
    if (BetweenVersion(version, 24.5, 24.5) || version >= 27.1)
    {
        if (!TryReadU32(pe, va + 12, value))
        {
            return false;
        }
        out.indices.adjustorThunk = static_cast<int32_t>(value);
    }
    return true;
}

} // namespace er2
