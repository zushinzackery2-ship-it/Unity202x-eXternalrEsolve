#include "GlobalStringXrefDocumentWriter.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace er2
{

namespace
{

std::string ToHex(std::uintptr_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << value;
    return stream.str();
}

void WriteJsonString(std::ostream& output, const std::string& value)
{
    output << '"';
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 0x20)
            {
                const auto flags = output.flags();
                const char fill = output.fill();
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned int>(character);
                output.flags(flags);
                output.fill(fill);
            }
            else
            {
                output << static_cast<char>(character);
            }
            break;
        }
    }
    output << '"';
}

std::string ReportName(const std::filesystem::path& outputPath)
{
    return outputPath.filename().string();
}

} // namespace

bool GlobalStringXrefDocumentWriter::Write(
    const PeImage& image,
    const std::filesystem::path& outputPath,
    const GlobalStringXrefOptions& options,
    const std::vector<DetectedGlobalString>& strings,
    const std::vector<std::size_t>& outputIndices,
    const char* sectionFilter,
    GlobalStringXrefResult& result,
    std::string& error)
{
    std::error_code filesystemError;
    if (outputPath.has_parent_path())
    {
        std::filesystem::create_directories(outputPath.parent_path(), filesystemError);
        if (filesystemError)
        {
            error = "failed to create global string xref output directory: " + filesystemError.message();
            return false;
        }
    }

    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        error = "failed to open " + ReportName(outputPath);
        return false;
    }

    result = {};
    result.stringCount = outputIndices.size();
    for (const std::size_t index : outputIndices)
    {
        result.referenceCount += strings[index].references.size();
    }

    output << "{\n";
    output << "  \"ImageBase\": ";
    WriteJsonString(output, ToHex(static_cast<std::uintptr_t>(image.ImageBase())));
    output << ",\n  \"Architecture\": \"x64\",\n";
    if (sectionFilter != nullptr)
    {
        output << "  \"SectionFilter\": ";
        WriteJsonString(output, sectionFilter);
        output << ",\n";
    }
    output << "  \"MinimumLength\": " << options.minimumLength << ",\n";
    output << "  \"MaximumByteLength\": " << options.maximumByteLength << ",\n";
    output << "  \"StringCount\": " << result.stringCount << ",\n";
    output << "  \"ReferenceCount\": " << result.referenceCount << ",\n";
    output << "  \"Strings\": [";

    for (std::size_t outputIndex = 0; outputIndex < outputIndices.size(); ++outputIndex)
    {
        const DetectedGlobalString& detected = strings[outputIndices[outputIndex]];
        output << (outputIndex == 0 ? "\n" : ",\n") << "    {\n";
        output << "      \"Address\": ";
        WriteJsonString(output, ToHex(detected.address));
        output << ",\n      \"Rva\": ";
        WriteJsonString(output, ToHex(detected.rva));
        output << ",\n      \"Section\": ";
        WriteJsonString(output, detected.section);
        output << ",\n      \"Encoding\": ";
        WriteJsonString(output, detected.encoding);
        output << ",\n      \"ByteLength\": " << detected.byteLength << ",\n";
        output << "      \"Value\": ";
        WriteJsonString(output, detected.value);
        output << ",\n      \"References\": [";

        for (std::size_t referenceIndex = 0;
             referenceIndex < detected.references.size();
             ++referenceIndex)
        {
            const GlobalStringReference& reference = detected.references[referenceIndex];
            output << (referenceIndex == 0 ? "\n" : ",\n") << "        {\n";
            output << "          \"Rva\": ";
            WriteJsonString(output, ToHex(reference.rva));
            output << ",\n          \"TargetAddress\": ";
            WriteJsonString(output, ToHex(reference.targetAddress));
            output << ",\n          \"Section\": ";
            WriteJsonString(output, reference.section);
            output << ",\n          \"Kind\": ";
            WriteJsonString(output, reference.kind);
            output << ",\n          \"Mnemonic\": ";
            WriteJsonString(output, reference.mnemonic);
            output << "\n        }";
        }
        if (!detected.references.empty())
        {
            output << '\n';
        }
        output << "      ]\n    }";
    }
    if (!outputIndices.empty())
    {
        output << '\n';
    }
    output << "  ]\n}\n";

    if (!output)
    {
        error = "failed to write " + ReportName(outputPath);
        return false;
    }
    return true;
}

} // namespace er2
