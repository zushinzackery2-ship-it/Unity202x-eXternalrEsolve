#include <er2/unity2/dumpsdk/offline/OfflineRuntimeContext.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

#include <format>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

bool OfflineRuntimeContext::LoadTypes(
    const MetadataRegistrationView& metaReg,
    std::string& error)
{
    const size_t count = static_cast<size_t>(metaReg.typesCount);
    types_.assign(count, Il2CppTypeRuntime{});
    typePtrToIndex_.clear();
    typePtrToIndex_.reserve(count * 2);

    size_t failures = 0;
    for (size_t i = 0; i < count; ++i)
    {
        uint64_t typePtr = 0;
        if (!TryReadU64(pe_, metaReg.types + i * kPtrSize, typePtr) ||
            typePtr == 0 ||
            !ReadIl2CppType(pe_, static_cast<uintptr_t>(typePtr), version_, types_[i]))
        {
            ++failures;
            continue;
        }
        typePtrToIndex_[static_cast<uintptr_t>(typePtr)] = i;
    }
    if (failures * 2 > count)
    {
        error = std::format("types table unreadable ({} of {} entries failed)", failures, count);
        return false;
    }
    return true;
}

bool OfflineRuntimeContext::LoadFieldOffsets(const MetadataRegistrationView& metaReg)
{
    fieldOffsetsArePointers_ = version_ > 21.0;
    if (version_ == 21.0 && metaReg.fieldOffsets != 0)
    {
        uint32_t probe[6] = {};
        if (ReadAbs(pe_, metaReg.fieldOffsets, probe, sizeof(probe)))
        {
            fieldOffsetsArePointers_ =
                probe[0] == 0 && probe[1] == 0 && probe[2] == 0 &&
                probe[3] == 0 && probe[4] == 0 && probe[5] > 0;
        }
    }

    fieldOffsets_.clear();
    if (metaReg.fieldOffsets == 0 || metaReg.fieldOffsetsCount <= 0)
    {
        return true;
    }
    const size_t count = static_cast<size_t>(metaReg.fieldOffsetsCount);
    fieldOffsets_.assign(count, 0);
    for (size_t i = 0; i < count; ++i)
    {
        if (fieldOffsetsArePointers_)
        {
            TryReadU64(pe_, metaReg.fieldOffsets + i * kPtrSize, fieldOffsets_[i]);
        }
        else
        {
            uint32_t value = 0;
            TryReadU32(pe_, metaReg.fieldOffsets + i * sizeof(uint32_t), value);
            fieldOffsets_[i] = value;
        }
    }
    return true;
}

bool OfflineRuntimeContext::TryGetTypeByPointer(uintptr_t pointer, Il2CppTypeRuntime& out) const
{
    const auto found = typePtrToIndex_.find(pointer);
    if (found != typePtrToIndex_.end())
    {
        out = types_[found->second];
        return true;
    }
    return ReadIl2CppType(pe_, pointer, version_, out);
}

const Il2CppTypeRuntime* OfflineRuntimeContext::GetTypeByIndex(int64_t index) const
{
    if (index < 0 || static_cast<size_t>(index) >= types_.size())
    {
        return nullptr;
    }
    return &types_[static_cast<size_t>(index)];
}

int32_t OfflineRuntimeContext::GetFieldOffset(
    int32_t typeIndex,
    int32_t fieldIndexInType,
    int32_t flatFieldIndex,
    bool isValueType,
    bool isStatic) const
{
    int32_t offset = -1;
    if (fieldOffsetsArePointers_)
    {
        if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= fieldOffsets_.size())
        {
            return -1;
        }
        const uint64_t pointer = fieldOffsets_[static_cast<size_t>(typeIndex)];
        uint32_t value = 0;
        if (pointer == 0 ||
            !TryReadU32(pe_, pointer + static_cast<uint64_t>(fieldIndexInType) * sizeof(uint32_t), value))
        {
            return -1;
        }
        offset = static_cast<int32_t>(value);
    }
    else
    {
        if (flatFieldIndex < 0 || static_cast<size_t>(flatFieldIndex) >= fieldOffsets_.size())
        {
            return -1;
        }
        offset = static_cast<int32_t>(fieldOffsets_[static_cast<size_t>(flatFieldIndex)]);
    }

    if (offset > 0 && isValueType && !isStatic)
    {
        offset -= 16;
    }
    return offset;
}

} // namespace er2
