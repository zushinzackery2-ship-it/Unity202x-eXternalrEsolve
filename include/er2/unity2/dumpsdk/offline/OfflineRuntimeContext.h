#pragma once

#include <er2/unity2/dumpsdk/offline/Metadata.h>
#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/offline/RegistrationSearch.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace er2
{

class OfflineRuntimeContext
{
public:
    OfflineRuntimeContext(
        const PeImage& pe,
        const Metadata& metadata,
        uintptr_t metadataVirtualAddress);

    bool Init(const RegistrationInitResult& registration, std::string& error);

    double Version() const
    {
        return version_;
    }

    uintptr_t MetadataVirtualAddress() const
    {
        return metadataVirtualAddress_;
    }

    uintptr_t CodeRegistrationVa() const
    {
        return codeRegistrationVa_;
    }

    uintptr_t MetadataRegistrationVa() const
    {
        return metadataRegistrationVa_;
    }

    bool IsDumped() const
    {
        return isDumped_;
    }

    const PeImage& Pe() const
    {
        return pe_;
    }

    bool TryGetTypeByPointer(uintptr_t pointer, Il2CppTypeRuntime& out) const;
    const Il2CppTypeRuntime* GetTypeByIndex(int64_t index) const;

    int32_t GetFieldOffset(
        int32_t typeIndex,
        int32_t fieldIndexInType,
        int32_t flatFieldIndex,
        bool isValueType,
        bool isStatic) const;

    uintptr_t GetMethodPointer(
        const std::string& imageName,
        const Il2CppMethodDefinition& methodDef) const;

    uintptr_t GetCustomAttributeGenerator(int32_t attributeIndex) const;

    const std::vector<uintptr_t>& MetadataUsageAddresses() const
    {
        return metadataUsages_;
    }

    uintptr_t MetadataUsageSlotAddress(uint32_t destinationIndex) const;

    struct MethodSpecEntry
    {
        Il2CppMethodSpec spec{};
        uintptr_t genericMethodPointer = 0;
    };

    bool TryGetMethodSpec(int32_t methodSpecIndex, Il2CppMethodSpec& out) const;
    uintptr_t GetGenericMethodPointerForSpec(int32_t methodSpecIndex) const;
    const std::vector<MethodSpecEntry>* GetMethodSpecsForMethod(int32_t methodDefinitionIndex) const;

    const std::vector<Il2CppGenericInst>& GenericInsts() const
    {
        return genericInsts_;
    }

private:
    bool LoadTypes(const MetadataRegistrationView& metaReg, std::string& error);
    bool LoadFieldOffsets(const MetadataRegistrationView& metaReg);
    bool LoadMethodPointers(const CodeRegistrationView& codeReg);
    bool LoadGenericData(const CodeRegistrationView& codeReg, const MetadataRegistrationView& metaReg);
    void LoadMetadataUsages(const MetadataRegistrationView& metaReg);
    void LoadCustomAttributeGenerators(const CodeRegistrationView& codeReg);

    const PeImage& pe_;
    const Metadata& metadata_;
    uintptr_t metadataVirtualAddress_ = 0;
    double version_ = 0.0;
    uintptr_t codeRegistrationVa_ = 0;
    uintptr_t metadataRegistrationVa_ = 0;
    bool isDumped_ = false;
    bool fieldOffsetsArePointers_ = false;

    std::vector<Il2CppTypeRuntime> types_;
    std::unordered_map<uintptr_t, size_t> typePtrToIndex_;
    std::vector<uint64_t> fieldOffsets_;
    std::vector<uintptr_t> legacyMethodPointers_;
    std::unordered_map<std::string, std::vector<uintptr_t>> codeGenModuleMethodPointers_;
    std::unordered_map<std::string, CodeGenModuleView> codeGenModules_;
    std::vector<uintptr_t> genericMethodPointers_;
    std::vector<uintptr_t> genericInstPointers_;
    std::vector<Il2CppGenericInst> genericInsts_;
    std::unordered_map<int32_t, std::vector<MethodSpecEntry>> methodDefinitionMethodSpecs_;
    std::vector<uintptr_t> metadataUsages_;
    uintptr_t metadataUsagesVa_ = 0;
    std::vector<Il2CppMethodSpec> methodSpecs_;
    std::vector<uintptr_t> methodSpecGenericPointers_;
    std::vector<uintptr_t> customAttributeGenerators_;
};

} // namespace er2
