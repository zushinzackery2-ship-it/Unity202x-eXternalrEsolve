#pragma once

#include <cstddef>
#include <cstdint>

namespace er2
{

enum class Il2CppTypeEnum : uint8_t
{
    IL2CPP_TYPE_END = 0x00,
    IL2CPP_TYPE_VOID = 0x01,
    IL2CPP_TYPE_BOOLEAN = 0x02,
    IL2CPP_TYPE_CHAR = 0x03,
    IL2CPP_TYPE_I1 = 0x04,
    IL2CPP_TYPE_U1 = 0x05,
    IL2CPP_TYPE_I2 = 0x06,
    IL2CPP_TYPE_U2 = 0x07,
    IL2CPP_TYPE_I4 = 0x08,
    IL2CPP_TYPE_U4 = 0x09,
    IL2CPP_TYPE_I8 = 0x0A,
    IL2CPP_TYPE_U8 = 0x0B,
    IL2CPP_TYPE_R4 = 0x0C,
    IL2CPP_TYPE_R8 = 0x0D,
    IL2CPP_TYPE_STRING = 0x0E,
    IL2CPP_TYPE_PTR = 0x0F,
    IL2CPP_TYPE_BYREF = 0x10,
    IL2CPP_TYPE_VALUETYPE = 0x11,
    IL2CPP_TYPE_CLASS = 0x12,
    IL2CPP_TYPE_VAR = 0x13,
    IL2CPP_TYPE_ARRAY = 0x14,
    IL2CPP_TYPE_GENERICINST = 0x15,
    IL2CPP_TYPE_TYPEDBYREF = 0x16,
    IL2CPP_TYPE_I = 0x18,
    IL2CPP_TYPE_U = 0x19,
    IL2CPP_TYPE_FNPTR = 0x1B,
    IL2CPP_TYPE_OBJECT = 0x1C,
    IL2CPP_TYPE_SZARRAY = 0x1D,
    IL2CPP_TYPE_MVAR = 0x1E,
    IL2CPP_TYPE_CMOD_REQD = 0x1F,
    IL2CPP_TYPE_CMOD_OPT = 0x20,
    IL2CPP_TYPE_INTERNAL = 0x21,
    IL2CPP_TYPE_MODIFIER = 0x40,
    IL2CPP_TYPE_SENTINEL = 0x41,
    IL2CPP_TYPE_PINNED = 0x45,
    IL2CPP_TYPE_ENUM = 0x55,
    IL2CPP_TYPE_INDEX = 0xFF,
};

struct Il2CppTypeRaw
{
    uint64_t datapoint = 0;
    uint32_t bits = 0;
};

struct Il2CppTypeRuntime
{
    uint64_t datapoint = 0;
    uint32_t bits = 0;
    uint32_t attrs = 0;
    Il2CppTypeEnum type = Il2CppTypeEnum::IL2CPP_TYPE_END;
    uint32_t num_mods = 0;
    uint32_t byref = 0;
    uint32_t pinned = 0;
    uint32_t valuetype = 0;

    void Init(double version);
    int64_t KlassIndex() const
    {
        return static_cast<int64_t>(datapoint);
    }
    uint64_t TypeHandle() const
    {
        return datapoint;
    }
    uint64_t NestedType() const
    {
        return datapoint;
    }
    uint64_t ArrayType() const
    {
        return datapoint;
    }
    int64_t GenericParameterIndex() const
    {
        return static_cast<int64_t>(datapoint);
    }
    uint64_t GenericParameterHandle() const
    {
        return datapoint;
    }
    uint64_t GenericClass() const
    {
        return datapoint;
    }
};

struct Il2CppArrayType
{
    uint64_t etype = 0;
    uint8_t rank = 0;
    uint8_t numsizes = 0;
    uint8_t numlobounds = 0;
    uint8_t padding = 0;
    uint64_t sizes = 0;
    uint64_t lobounds = 0;
};

struct Il2CppGenericInst
{
    int64_t type_argc = 0;
    uint64_t type_argv = 0;
};

struct Il2CppGenericContext
{
    uint64_t class_inst = 0;
    uint64_t method_inst = 0;
};

struct Il2CppGenericClass
{
    int64_t typeDefinitionIndex = 0;
    uint64_t type = 0;
    Il2CppGenericContext context{};
    uint64_t cached_class = 0;
};

struct Il2CppMethodSpec
{
    int32_t methodDefinitionIndex = 0;
    int32_t classIndexIndex = 0;
    int32_t methodIndexIndex = 0;
};

struct Il2CppGenericMethodIndices
{
    int32_t methodIndex = 0;
    int32_t invokerIndex = 0;
    int32_t adjustorThunk = 0;
};

struct Il2CppGenericMethodFunctions
{
    int32_t genericMethodIndex = 0;
    Il2CppGenericMethodIndices indices{};
};

struct CodeRegistrationView
{
    uint64_t methodPointersCount = 0;
    uint64_t methodPointers = 0;
    uint64_t reversePInvokeWrapperCount = 0;
    uint64_t reversePInvokeWrappers = 0;
    uint64_t genericMethodPointersCount = 0;
    uint64_t genericMethodPointers = 0;
    uint64_t genericAdjustorThunks = 0;
    uint64_t invokerPointersCount = 0;
    uint64_t invokerPointers = 0;
    uint64_t customAttributeCount = 0;
    uint64_t customAttributeGenerators = 0;
    uint64_t unresolvedVirtualCallCount = 0;
    uint64_t unresolvedVirtualCallPointers = 0;
    uint64_t unresolvedInstanceCallPointers = 0;
    uint64_t unresolvedStaticCallPointers = 0;
    uint64_t interopDataCount = 0;
    uint64_t interopData = 0;
    uint64_t windowsRuntimeFactoryCount = 0;
    uint64_t windowsRuntimeFactoryTable = 0;
    uint64_t codeGenModulesCount = 0;
    uint64_t codeGenModules = 0;
};

struct MetadataRegistrationView
{
    int64_t genericClassesCount = 0;
    uint64_t genericClasses = 0;
    int64_t genericInstsCount = 0;
    uint64_t genericInsts = 0;
    int64_t genericMethodTableCount = 0;
    uint64_t genericMethodTable = 0;
    int64_t typesCount = 0;
    uint64_t types = 0;
    int64_t methodSpecsCount = 0;
    uint64_t methodSpecs = 0;
    int64_t fieldOffsetsCount = 0;
    uint64_t fieldOffsets = 0;
    int64_t typeDefinitionsSizesCount = 0;
    uint64_t typeDefinitionsSizes = 0;
    uint64_t metadataUsagesCount = 0;
    uint64_t metadataUsages = 0;
};

struct CodeGenModuleView
{
    uint64_t moduleName = 0;
    int64_t methodPointerCount = 0;
    uint64_t methodPointers = 0;
    int64_t adjustorThunkCount = 0;
    uint64_t adjustorThunks = 0;
    uint64_t invokerIndices = 0;
    uint64_t reversePInvokeWrapperCount = 0;
    uint64_t reversePInvokeWrapperIndices = 0;
    int64_t rgctxRangesCount = 0;
    uint64_t rgctxRanges = 0;
    int64_t rgctxsCount = 0;
    uint64_t rgctxs = 0;
    uint64_t debuggerMetadata = 0;
    uint64_t customAttributeCacheGenerator = 0;
};

bool ReadCodeRegistration(const class PeImage& pe, uintptr_t va, double version, CodeRegistrationView& out);
bool ReadMetadataRegistration(const class PeImage& pe, uintptr_t va, double version, MetadataRegistrationView& out);
bool ReadCodeGenModule(const class PeImage& pe, uintptr_t va, double version, CodeGenModuleView& out);
bool ReadIl2CppType(const class PeImage& pe, uintptr_t va, double version, Il2CppTypeRuntime& out);
bool ReadIl2CppGenericClass(const class PeImage& pe, uintptr_t va, double version, Il2CppGenericClass& out);
bool ReadIl2CppArrayType(const class PeImage& pe, uintptr_t va, Il2CppArrayType& out);
bool ReadIl2CppGenericInst(const class PeImage& pe, uintptr_t va, Il2CppGenericInst& out);
bool ReadIl2CppMethodSpec(const class PeImage& pe, uintptr_t va, Il2CppMethodSpec& out);
bool ReadGenericMethodFunctions(
    const class PeImage& pe,
    uintptr_t va,
    double version,
    Il2CppGenericMethodFunctions& out);

size_t MethodSpecSize();
size_t GenericMethodFunctionsSize(double version);

} // namespace er2
