#include <er2/unity2/dumpsdk/offline/OfflineRuntimeContext.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

bool OfflineRuntimeContext::LoadGenericData(
    const CodeRegistrationView& codeReg,
    const MetadataRegistrationView& metaReg)
{
    genericMethodPointers_.clear();
    genericInstPointers_.clear();
    genericInsts_.clear();
    methodSpecs_.clear();
    methodSpecGenericPointers_.clear();
    methodDefinitionMethodSpecs_.clear();

    if (codeReg.genericMethodPointers != 0 && codeReg.genericMethodPointersCount > 0 &&
        codeReg.genericMethodPointersCount < 0x400000)
    {
        genericMethodPointers_.assign(static_cast<size_t>(codeReg.genericMethodPointersCount), 0);
        for (uint64_t i = 0; i < codeReg.genericMethodPointersCount; ++i)
        {
            uint64_t pointer = 0;
            TryReadU64(pe_, codeReg.genericMethodPointers + i * kPtrSize, pointer);
            genericMethodPointers_[static_cast<size_t>(i)] = static_cast<uintptr_t>(pointer);
        }
    }

    if (metaReg.genericInsts != 0 && metaReg.genericInstsCount > 0 &&
        metaReg.genericInstsCount < 0x400000)
    {
        const size_t count = static_cast<size_t>(metaReg.genericInstsCount);
        genericInstPointers_.assign(count, 0);
        genericInsts_.assign(count, Il2CppGenericInst{});
        for (size_t i = 0; i < count; ++i)
        {
            uint64_t pointer = 0;
            if (!TryReadU64(pe_, metaReg.genericInsts + i * kPtrSize, pointer) || pointer == 0)
            {
                continue;
            }
            genericInstPointers_[i] = static_cast<uintptr_t>(pointer);
            ReadIl2CppGenericInst(pe_, static_cast<uintptr_t>(pointer), genericInsts_[i]);
        }
    }

    if (metaReg.methodSpecs == 0 || metaReg.methodSpecsCount <= 0 ||
        metaReg.genericMethodTable == 0 || metaReg.genericMethodTableCount <= 0 ||
        metaReg.methodSpecsCount > 0x400000 || metaReg.genericMethodTableCount > 0x400000)
    {
        return true;
    }

    methodSpecs_.assign(static_cast<size_t>(metaReg.methodSpecsCount), Il2CppMethodSpec{});
    for (size_t i = 0; i < methodSpecs_.size(); ++i)
    {
        ReadIl2CppMethodSpec(pe_, metaReg.methodSpecs + i * MethodSpecSize(), methodSpecs_[i]);
    }
    methodSpecGenericPointers_.assign(methodSpecs_.size(), 0);

    const size_t entrySize = GenericMethodFunctionsSize(version_);
    for (int64_t i = 0; i < metaReg.genericMethodTableCount; ++i)
    {
        Il2CppGenericMethodFunctions table{};
        if (!ReadGenericMethodFunctions(
                pe_,
                metaReg.genericMethodTable + static_cast<uint64_t>(i) * entrySize,
                version_,
                table) ||
            table.genericMethodIndex < 0 ||
            static_cast<size_t>(table.genericMethodIndex) >= methodSpecs_.size())
        {
            continue;
        }

        const Il2CppMethodSpec& spec = methodSpecs_[static_cast<size_t>(table.genericMethodIndex)];
        uintptr_t pointer = 0;
        if (table.indices.methodIndex >= 0 &&
            static_cast<size_t>(table.indices.methodIndex) < genericMethodPointers_.size())
        {
            pointer = genericMethodPointers_[static_cast<size_t>(table.indices.methodIndex)];
        }
        methodSpecGenericPointers_[static_cast<size_t>(table.genericMethodIndex)] = pointer;
        methodDefinitionMethodSpecs_[spec.methodDefinitionIndex].push_back({ spec, pointer });
    }
    return true;
}

void OfflineRuntimeContext::LoadMetadataUsages(const MetadataRegistrationView& metaReg)
{
    metadataUsages_.clear();
    metadataUsagesVa_ = 0;
    if (version_ <= 16.0 || version_ >= 27.0)
    {
        return;
    }
    const int64_t count = metadata_.MetadataUsagesCount();
    if (metaReg.metadataUsages == 0 || count <= 0 || count > 0x400000)
    {
        return;
    }
    metadataUsagesVa_ = static_cast<uintptr_t>(metaReg.metadataUsages);
    metadataUsages_.assign(static_cast<size_t>(count), 0);
    for (int64_t i = 0; i < count; ++i)
    {
        uint64_t pointer = 0;
        TryReadU64(pe_, metaReg.metadataUsages + static_cast<uint64_t>(i) * kPtrSize, pointer);
        metadataUsages_[static_cast<size_t>(i)] = static_cast<uintptr_t>(pointer);
    }
}

} // namespace er2
