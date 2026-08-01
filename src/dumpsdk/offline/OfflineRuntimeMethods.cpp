#include <er2/unity2/dumpsdk/offline/OfflineRuntimeContext.h>

#include <er2/unity2/dumpsdk/offline/PeImageAccess.h>

namespace er2
{

namespace
{

constexpr size_t kPtrSize = 8;

} // namespace

bool OfflineRuntimeContext::LoadMethodPointers(const CodeRegistrationView& codeReg)
{
    legacyMethodPointers_.clear();
    codeGenModuleMethodPointers_.clear();
    codeGenModules_.clear();

    if (version_ >= 24.2)
    {
        if (codeReg.codeGenModules == 0 || codeReg.codeGenModulesCount == 0 ||
            codeReg.codeGenModulesCount > 0x400000)
        {
            return false;
        }
        for (uint64_t i = 0; i < codeReg.codeGenModulesCount; ++i)
        {
            uint64_t modulePtr = 0;
            if (!TryReadU64(pe_, codeReg.codeGenModules + i * kPtrSize, modulePtr) || modulePtr == 0)
            {
                continue;
            }
            CodeGenModuleView module{};
            if (!ReadCodeGenModule(pe_, static_cast<uintptr_t>(modulePtr), version_, module))
            {
                continue;
            }
            const std::string moduleName = pe_.ReadCString(module.moduleName);
            if (moduleName.empty())
            {
                continue;
            }

            std::vector<uintptr_t> pointers;
            if (module.methodPointerCount > 0 && module.methodPointerCount < 0x400000 &&
                module.methodPointers != 0)
            {
                pointers.assign(static_cast<size_t>(module.methodPointerCount), 0);
                for (int64_t j = 0; j < module.methodPointerCount; ++j)
                {
                    uint64_t pointer = 0;
                    TryReadU64(pe_, module.methodPointers + static_cast<uint64_t>(j) * kPtrSize, pointer);
                    pointers[static_cast<size_t>(j)] = static_cast<uintptr_t>(pointer);
                }
            }
            codeGenModules_[moduleName] = module;
            codeGenModuleMethodPointers_[moduleName] = std::move(pointers);
        }
        return !codeGenModuleMethodPointers_.empty();
    }

    if (codeReg.methodPointers == 0 || codeReg.methodPointersCount == 0 ||
        codeReg.methodPointersCount > 0x400000)
    {
        return false;
    }
    legacyMethodPointers_.assign(static_cast<size_t>(codeReg.methodPointersCount), 0);
    for (uint64_t i = 0; i < codeReg.methodPointersCount; ++i)
    {
        uint64_t pointer = 0;
        TryReadU64(pe_, codeReg.methodPointers + i * kPtrSize, pointer);
        legacyMethodPointers_[static_cast<size_t>(i)] = static_cast<uintptr_t>(pointer);
    }
    return true;
}

uintptr_t OfflineRuntimeContext::GetMethodPointer(
    const std::string& imageName,
    const Il2CppMethodDefinition& methodDef) const
{
    if (version_ >= 24.2)
    {
        const auto found = codeGenModuleMethodPointers_.find(imageName);
        const uint32_t tokenIndex = methodDef.token & 0x00FFFFFFu;
        if (found == codeGenModuleMethodPointers_.end() || tokenIndex == 0)
        {
            return 0;
        }
        const size_t index = static_cast<size_t>(tokenIndex) - 1;
        return index < found->second.size() ? found->second[index] : 0;
    }
    if (methodDef.methodIndex < 0 ||
        static_cast<size_t>(methodDef.methodIndex) >= legacyMethodPointers_.size())
    {
        return 0;
    }
    return legacyMethodPointers_[static_cast<size_t>(methodDef.methodIndex)];
}

void OfflineRuntimeContext::LoadCustomAttributeGenerators(const CodeRegistrationView& codeReg)
{
    customAttributeGenerators_.clear();
    if (version_ < 21.0 || version_ >= 29.0)
    {
        return;
    }
    if (version_ < 27.0)
    {
        if (codeReg.customAttributeGenerators == 0 || codeReg.customAttributeCount == 0 ||
            codeReg.customAttributeCount > 0x400000)
        {
            return;
        }
        customAttributeGenerators_.assign(static_cast<size_t>(codeReg.customAttributeCount), 0);
        for (uint64_t i = 0; i < codeReg.customAttributeCount; ++i)
        {
            uint64_t pointer = 0;
            TryReadU64(pe_, codeReg.customAttributeGenerators + i * kPtrSize, pointer);
            customAttributeGenerators_[static_cast<size_t>(i)] = static_cast<uintptr_t>(pointer);
        }
        return;
    }

    uint64_t total = 0;
    for (const Il2CppImageDefinition& imageDef : metadata_.ImageDefs())
    {
        total += imageDef.customAttributeCount;
    }
    if (total == 0 || total > 0x400000)
    {
        return;
    }
    customAttributeGenerators_.assign(static_cast<size_t>(total), 0);
    for (const Il2CppImageDefinition& imageDef : metadata_.ImageDefs())
    {
        const std::string imageName = metadata_.GetStringFromIndex(imageDef.nameIndex);
        const auto module = codeGenModules_.find(imageName);
        if (module == codeGenModules_.end() || module->second.customAttributeCacheGenerator == 0)
        {
            continue;
        }
        for (uint32_t i = 0; i < imageDef.customAttributeCount; ++i)
        {
            const size_t target = static_cast<size_t>(imageDef.customAttributeStart) + i;
            if (target >= customAttributeGenerators_.size())
            {
                break;
            }
            uint64_t pointer = 0;
            TryReadU64(pe_, module->second.customAttributeCacheGenerator +
                static_cast<uint64_t>(i) * kPtrSize, pointer);
            customAttributeGenerators_[target] = static_cast<uintptr_t>(pointer);
        }
    }
}

} // namespace er2
