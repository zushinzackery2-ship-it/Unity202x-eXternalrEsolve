#pragma once

#include <cstdint>
#include <vector>

namespace OfflineBehavior
{

struct MetadataBlob
{
    std::vector<uint8_t> bytes;
    uint32_t typeDefinitionsOffset = 0;
    uint32_t genericParametersOffset = 0;
    int32_t typeDefCount = 0;
    int32_t methodCount = 0;
    int32_t imageCount = 0;
};

MetadataBlob BuildMetadata();

constexpr uint32_t kTypeDefinitionSize = 88;
constexpr uint32_t kGenericParameterSize = 16;
constexpr const char* kImageName = "mscorlib.dll";

enum SmokeTypeIndex
{
    kTypeObject = 0,
    kTypeValueType = 1,
    kTypeEnum = 2,
    kTypeInt32 = 3,
    kTypeString = 4,
    kTypeIDisposable = 5,
    kTypeBase = 6,
    kTypeDerived = 7,
    kTypeNested = 8,
    kTypeGeneric = 9,
    kTypeAttribute = 10,
    kTypeCount = 11,
};

enum SmokeRuntimeType
{
    kRtObject = 0,
    kRtValueType = 1,
    kRtEnum = 2,
    kRtInt32 = 3,
    kRtString = 4,
    kRtIDisposable = 5,
    kRtBase = 6,
    kRtDerived = 7,
    kRtNested = 8,
    kRtGeneric = 9,
    kRtAttribute = 10,
    kRtVoid = 11,
    kRtVar0 = 12,
    kRtFieldCounter = 13,
    kRtFieldTag = 14,
    kRtFieldAnswer = 15,
    kRtFieldValues = 16,
    kRtParamOutInt = 17,
    kRtParamRefInt = 18,
    kRtParamInInt = 19,
    kRtFieldMatrix = 20,
    kRtFieldGeneric = 21,
    kRtFieldNoOffset = 22,
    kRtMVar0 = 23,
    kRtCount = 24,
};

enum SmokeMethodIndex
{
    kMethodBaseRun = 0,
    kMethodGetCounter = 1,
    kMethodSetCounter = 2,
    kMethodAddChanged = 3,
    kMethodRemoveChanged = 4,
    kMethodDerivedRun = 5,
    kMethodCompute = 6,
    kMethodDerivedCtor = 7,
    kMethodEcho = 8,
    kMethodContainerGet = 9,
    kMethodAttributeCtor = 10,
    kMethodCount = 11,
};

} // namespace OfflineBehavior
