#pragma once

#if defined(_WIN32) || defined(_WIN64)

#include <cstdint>
#include <cstring>

namespace UnityExternal
{

namespace detail_metadata
{

struct DiskSection
{
    char name[9];
    std::uint32_t rva;
    std::uint32_t vsize;
};

struct Il2CppRegs
{
    std::uintptr_t codeRegistration = 0;
    std::uintptr_t metadataRegistration = 0;
};

struct CodeRegOffsets
{
    int invokerPointersCount = -1;
    int invokerPointers = -1;
    int codeGenModulesCount = -1;
    int codeGenModules = -1;
    int needBytes = 0;
};

struct MetaRegOffsets
{
    int typesCount = -1;
    int types = -1;
    int fieldOffsetsCount = -1;
    int fieldOffsets = -1;
    int typeDefinitionsSizesCount = -1;
    int typeDefinitionsSizes = -1;
    int structSize = 0;
};

inline CodeRegOffsets GetCodeRegistrationOffsets(std::uint32_t version)
{
    struct Field
    {
        const char* name;
        double vmin;
        double vmax;
        bool hasMin;
        bool hasMax;
    };

    static const Field fields[] = {
        {"methodPointersCount", 0.0, 24.1, false, true},
        {"methodPointers", 0.0, 24.1, false, true},
        {"delegateWrappersFromNativeToManagedCount", 0.0, 21.0, false, true},
        {"delegateWrappersFromNativeToManaged", 0.0, 21.0, false, true},
        {"reversePInvokeWrapperCount", 22.0, 0.0, true, false},
        {"reversePInvokeWrappers", 22.0, 0.0, true, false},
        {"delegateWrappersFromManagedToNativeCount", 0.0, 22.0, false, true},
        {"delegateWrappersFromManagedToNative", 0.0, 22.0, false, true},
        {"marshalingFunctionsCount", 0.0, 22.0, false, true},
        {"marshalingFunctions", 0.0, 22.0, false, true},
        {"ccwMarshalingFunctionsCount", 21.0, 22.0, true, true},
        {"ccwMarshalingFunctions", 21.0, 22.0, true, true},
        {"genericMethodPointersCount", 0.0, 0.0, false, false},
        {"genericMethodPointers", 0.0, 0.0, false, false},
        {"genericAdjustorThunks", 27.1, 0.0, true, false},
        {"invokerPointersCount", 0.0, 0.0, false, false},
        {"invokerPointers", 0.0, 0.0, false, false},
        {"customAttributeCount", 0.0, 24.5, false, true},
        {"customAttributeGenerators", 0.0, 24.5, false, true},
        {"guidCount", 21.0, 22.0, true, true},
        {"guids", 21.0, 22.0, true, true},
        {"unresolvedVirtualCallCount", 22.0, 0.0, true, false},
        {"unresolvedVirtualCallPointers", 22.0, 0.0, true, false},
        {"unresolvedInstanceCallPointers", 29.1, 30.99, true, true},
        {"unresolvedStaticCallPointers", 29.1, 30.99, true, true},
        {"interopDataCount", 23.0, 0.0, true, false},
        {"interopData", 23.0, 0.0, true, false},
        {"windowsRuntimeFactoryCount", 24.3, 0.0, true, false},
        {"windowsRuntimeFactoryTable", 24.3, 0.0, true, false},
        {"codeGenModulesCount", 24.2, 0.0, true, false},
        {"codeGenModules", 24.2, 0.0, true, false},
    };

    CodeRegOffsets out{};

    double v = (double)version;
    int off = 0;
    for (const auto& f : fields)
    {
        if (f.hasMin && v < f.vmin) continue;
        if (f.hasMax && v > f.vmax) continue;

        if (std::strcmp(f.name, "invokerPointersCount") == 0) out.invokerPointersCount = off;
        if (std::strcmp(f.name, "invokerPointers") == 0) out.invokerPointers = off;
        if (std::strcmp(f.name, "codeGenModulesCount") == 0) out.codeGenModulesCount = off;
        if (std::strcmp(f.name, "codeGenModules") == 0) out.codeGenModules = off;
        off += 8;
    }

    if (out.invokerPointersCount < 0 || out.invokerPointers < 0 || out.codeGenModulesCount < 0 || out.codeGenModules < 0)
    {
        out.needBytes = 0;
        return out;
    }

    out.needBytes = out.codeGenModules + 8;
    return out;
}

inline MetaRegOffsets GetMetadataRegistrationOffsets(std::uint32_t version)
{
    MetaRegOffsets out{};
    int off = 0;

    auto add = [&off](int& dst)
    {
        dst = off;
        off += 8;
    };

    int tmp = 0;
    add(tmp);
    add(tmp);
    add(tmp);
    add(tmp);
    add(tmp);
    add(tmp);

    add(out.typesCount);
    add(out.types);

    add(tmp);
    add(tmp);

    if (version <= 16)
    {
        add(tmp);
        add(tmp);
    }

    add(out.fieldOffsetsCount);
    add(out.fieldOffsets);
    add(out.typeDefinitionsSizesCount);
    add(out.typeDefinitionsSizes);

    if (version >= 19)
    {
        add(tmp);
        add(tmp);
    }

    out.structSize = off;
    return out;
}

}

}

#endif
