#include "SmokeMetadataInternal.h"

namespace OfflineBehavior
{

void BuildSmokeTables(StringTable& strings, SmokeTables& tables)
{
    BuildTypeAndMethodTables(strings, tables);

    {
        Writer writer;
        uint32_t paramIndex = 0;
        auto param = [&](const char* name, int32_t runtimeType)
        {
            writer.U32(strings.Add(name));
            writer.U32(kParamTokenBase + paramIndex);
            writer.I32(runtimeType);
            ++paramIndex;
        };
        param("value", kRtInt32);
        param("handler", kRtIDisposable);
        param("handler", kRtIDisposable);
        param("a", kRtInt32);
        param("b", kRtParamOutInt);
        param("c", kRtParamRefInt);
        param("d", kRtParamInInt);
        param("label", kRtString);
        param("value", kRtMVar0);
        param("tag", kRtString);
        tables.params = std::move(writer.bytes);
    }

    {
        Writer writer;
        auto field = [&](const char* name, int32_t runtimeType, uint32_t index)
        {
            writer.U32(strings.Add(name));
            writer.I32(runtimeType);
            writer.U32(kFieldTokenBase + index);
        };
        field("_counter", kRtFieldCounter, 0);
        field("Tag", kRtFieldTag, 1);
        field("Answer", kRtFieldAnswer, 2);
        field("Values", kRtFieldValues, 3);
        field("Matrix", kRtFieldMatrix, 4);
        field("GenericValue", kRtFieldGeneric, 5);
        field("UnknownOffset", kRtFieldNoOffset, 6);
        tables.fields = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.U32(strings.Add("Counter"));
        writer.I32(kMethodGetCounter - kMethodGetCounter);
        writer.I32(kMethodSetCounter - kMethodGetCounter);
        writer.U32(0);
        writer.U32(kPropertyToken);
        tables.properties = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.U32(strings.Add("Changed"));
        writer.I32(kRtIDisposable);
        writer.I32(kMethodAddChanged - kMethodGetCounter);
        writer.I32(kMethodRemoveChanged - kMethodGetCounter);
        writer.I32(-1);
        writer.U32(kEventToken);
        tables.events = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.I32(kRtIDisposable);
        tables.interfaces = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.I32(kTypeNested);
        tables.nestedTypes = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.U32(kMethodBaseRun);
        writer.U32(kMethodDerivedRun);
        tables.vtableMethods = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.I32(kTypeGeneric);
        writer.I32(1);
        writer.I32(0);
        writer.I32(0);
        writer.I32(kMethodEcho);
        writer.I32(1);
        writer.I32(1);
        writer.I32(1);
        tables.genericContainers = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.I32(0);
        writer.U32(strings.Add("T"));
        writer.U16(0);
        writer.U16(0);
        writer.U16(0);
        writer.U16(0);
        writer.I32(1);
        writer.U32(strings.Add("U"));
        writer.U16(0);
        writer.U16(0);
        writer.U16(0);
        writer.U16(0);
        tables.genericParameters = std::move(writer.bytes);
    }

    {
        Writer data;
        const uint32_t answerData = data.Position();
        data.CompressedI32(42);
        const uint32_t labelData = data.Position();
        data.CompressedI32(2);
        data.Text("hi");
        tables.defaultData = std::move(data.bytes);

        Writer fieldDefaults;
        fieldDefaults.I32(2);
        fieldDefaults.I32(kRtFieldAnswer);
        fieldDefaults.I32(static_cast<int32_t>(answerData));
        tables.fieldDefaults = std::move(fieldDefaults.bytes);

        Writer paramDefaults;
        paramDefaults.I32(7);
        paramDefaults.I32(kRtString);
        paramDefaults.I32(static_cast<int32_t>(labelData));
        tables.paramDefaults = std::move(paramDefaults.bytes);
    }

    {
        Writer data;
        const std::string first = "hello smoke";
        const std::string second = "second literal";
        const uint32_t firstIndex = data.Position();
        data.Text(first);
        const uint32_t secondIndex = data.Position();
        data.Text(second);
        tables.stringLiteralData = std::move(data.bytes);

        Writer index;
        index.U32(static_cast<uint32_t>(first.size()));
        index.I32(static_cast<int32_t>(firstIndex));
        index.U32(static_cast<uint32_t>(second.size()));
        index.I32(static_cast<int32_t>(secondIndex));
        tables.stringLiterals = std::move(index.bytes);
    }

    {
        Writer blob;
        blob.CompressedU32(1);
        blob.I32(kMethodAttributeCtor);
        blob.CompressedU32(1);
        blob.CompressedU32(0);
        blob.CompressedU32(0);
        blob.U8(0x0E);
        blob.CompressedI32(6);
        blob.Text("tagged");
        const uint32_t blobEnd = blob.Position();
        tables.attributeData = std::move(blob.bytes);

        Writer ranges;
        ranges.U32(kTypeTokenBase + kTypeDerived);
        ranges.U32(0);
        ranges.U32(0);
        ranges.U32(blobEnd);
        tables.attributeDataRanges = std::move(ranges.bytes);
    }

    {
        Writer writer;
        writer.U32(strings.Add(kImageName));
        writer.I32(0);
        writer.I32(0);
        writer.U32(kTypeCount);
        writer.I32(0);
        writer.U32(0);
        writer.I32(-1);
        writer.U32(kImageToken);
        writer.I32(0);
        writer.U32(1);
        tables.images = std::move(writer.bytes);
    }

    {
        Writer writer;
        writer.I32(0);
        writer.U32(kAssemblyToken);
        writer.I32(0);
        writer.I32(0);
        writer.U32(strings.Add("SmokeAssembly"));
        writer.U32(strings.Add(""));
        writer.U32(strings.Add(""));
        writer.U32(0);
        writer.I32(0);
        writer.U32(0);
        writer.I32(1);
        writer.I32(0);
        writer.I32(0);
        writer.I32(0);
        for (int i = 0; i < 8; ++i)
        {
            writer.U8(0);
        }
        tables.assemblies = std::move(writer.bytes);
    }
}

} // namespace OfflineBehavior
