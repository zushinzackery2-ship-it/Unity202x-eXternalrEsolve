#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefExporter.h>

#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

constexpr std::uintptr_t ImageBase = 0x180000000;
constexpr std::uint32_t TextRva = 0x1000;
constexpr std::uint32_t StringRva = 0x2020;
constexpr std::uint32_t WritableStringRva = 0x3020;

void WriteSection(
    IMAGE_SECTION_HEADER& section,
    const char* name,
    std::uint32_t virtualAddress,
    std::uint32_t size,
    std::uint32_t characteristics)
{
    std::memcpy(section.Name, name, std::min<std::size_t>(std::strlen(name), sizeof(section.Name)));
    section.Misc.VirtualSize = size;
    section.VirtualAddress = virtualAddress;
    section.SizeOfRawData = size;
    section.Characteristics = characteristics;
}

std::vector<std::uint8_t> CreateImage(
    const char* dataSectionName,
    const std::vector<std::uint8_t>& referencedBytes,
    bool addAllReferenceKinds)
{
    std::vector<std::uint8_t> image(0x3000, 0);
    IMAGE_DOS_HEADER dosHeader = {};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_lfanew = 0x80;
    std::memcpy(image.data(), &dosHeader, sizeof(dosHeader));

    IMAGE_NT_HEADERS64 ntHeaders = {};
    ntHeaders.Signature = IMAGE_NT_SIGNATURE;
    ntHeaders.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    ntHeaders.FileHeader.NumberOfSections = 2;
    ntHeaders.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    ntHeaders.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    ntHeaders.OptionalHeader.ImageBase = ImageBase;
    ntHeaders.OptionalHeader.SizeOfImage = static_cast<std::uint32_t>(image.size());
    ntHeaders.OptionalHeader.SizeOfHeaders = 0x200;
    std::memcpy(image.data() + dosHeader.e_lfanew, &ntHeaders, sizeof(ntHeaders));

    const std::size_t sectionOffset = dosHeader.e_lfanew
        + offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
        + ntHeaders.FileHeader.SizeOfOptionalHeader;
    IMAGE_SECTION_HEADER sections[2] = {};
    WriteSection(sections[0], ".text", TextRva, 0x200,
        IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE);
    WriteSection(sections[1], dataSectionName, 0x2000, 0x200,
        IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
    std::memcpy(image.data() + sectionOffset, sections, sizeof(sections));

    const std::int32_t displacement = static_cast<std::int32_t>(StringRva - (TextRva + 7));
    image[TextRva] = 0x48;
    image[TextRva + 1] = 0x8D;
    image[TextRva + 2] = 0x0D;
    std::memcpy(image.data() + TextRva + 3, &displacement, sizeof(displacement));

    if (addAllReferenceKinds)
    {
        const std::uint64_t targetAddress = ImageBase + StringRva;
        image[TextRva + 0x10] = 0x48;
        image[TextRva + 0x11] = 0xA1;
        std::memcpy(image.data() + TextRva + 0x12, &targetAddress, sizeof(targetAddress));
        image[TextRva + 0x20] = 0x48;
        image[TextRva + 0x21] = 0xB8;
        std::memcpy(image.data() + TextRva + 0x22, &targetAddress, sizeof(targetAddress));
    }

    std::fill(image.begin() + 0x2000, image.begin() + StringRva, static_cast<std::uint8_t>('P'));
    std::copy(referencedBytes.begin(), referencedBytes.end(), image.begin() + StringRva);
    image[StringRva + referencedBytes.size()] = 0;
    image[StringRva + referencedBytes.size() + 1] = 0;
    return image;
}

std::vector<std::uint8_t> CreateRuntimeSectionImage()
{
    std::vector<std::uint8_t> image(0x4000, 0);
    IMAGE_DOS_HEADER dosHeader = {};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_lfanew = 0x80;
    std::memcpy(image.data(), &dosHeader, sizeof(dosHeader));

    IMAGE_NT_HEADERS64 ntHeaders = {};
    ntHeaders.Signature = IMAGE_NT_SIGNATURE;
    ntHeaders.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    ntHeaders.FileHeader.NumberOfSections = 3;
    ntHeaders.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    ntHeaders.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    ntHeaders.OptionalHeader.ImageBase = ImageBase;
    ntHeaders.OptionalHeader.SizeOfImage = static_cast<std::uint32_t>(image.size());
    ntHeaders.OptionalHeader.SizeOfHeaders = 0x200;
    std::memcpy(image.data() + dosHeader.e_lfanew, &ntHeaders, sizeof(ntHeaders));

    const std::size_t sectionOffset = dosHeader.e_lfanew
        + offsetof(IMAGE_NT_HEADERS64, OptionalHeader)
        + ntHeaders.FileHeader.SizeOfOptionalHeader;
    IMAGE_SECTION_HEADER sections[3] = {};
    WriteSection(sections[0], ".text", TextRva, 0x200,
        IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE);
    WriteSection(sections[1], ".rdata", 0x2000, 0x200,
        IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
    WriteSection(sections[2], ".data", 0x3000, 0x200,
        IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA);
    std::memcpy(image.data() + sectionOffset, sections, sizeof(sections));

    auto writeRipReference = [&image](std::uint32_t instructionRva, std::uint32_t targetRva)
    {
        const std::int32_t displacement = static_cast<std::int32_t>(targetRva - (instructionRva + 7));
        image[instructionRva] = 0x48;
        image[instructionRva + 1] = 0x8D;
        image[instructionRva + 2] = 0x0D;
        std::memcpy(image.data() + instructionRva + 3, &displacement, sizeof(displacement));
    };
    writeRipReference(TextRva, StringRva);
    writeRipReference(TextRva + 0x10, WritableStringRva);

    const char runtimeRdata[] = "RuntimeRdataString";
    const char writableData[] = "WritableDataString";
    std::memcpy(image.data() + StringRva, runtimeRdata, sizeof(runtimeRdata));
    std::memcpy(image.data() + WritableStringRva, writableData, sizeof(writableData));
    return image;
}

std::string ReadAll(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

bool Contains(const std::string& text, const std::string& expected)
{
    return text.find(expected) != std::string::npos;
}

bool ExportImage(
    std::vector<std::uint8_t> imageBytes,
    const std::filesystem::path& outputDirectory,
    er2::GlobalStringXrefReportResults& reports,
    std::string& globalDocument,
    std::string& runtimeRdataDocument)
{
    er2::PeImage image;
    std::string error;
    try
    {
        image.Load(imageBytes.data(), imageBytes.size());
        image.LoadFromMemory(ImageBase);
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return false;
    }

    if (!er2::GlobalStringXrefExporter::ExportReports(
            image,
            outputDirectory,
            {},
            reports,
            error))
    {
        std::cerr << error << '\n';
        return false;
    }
    globalDocument = ReadAll(outputDirectory / "global-string-xrefs.json");
    runtimeRdataDocument = ReadAll(outputDirectory / "runtime-rdata-string-xrefs.json");
    return true;
}

} // namespace

int main()
{
    const std::filesystem::path outputDirectory = "out";
    std::filesystem::remove_all(outputDirectory);
    std::filesystem::create_directories(outputDirectory);

    er2::GlobalStringXrefReportResults reports;
    std::string globalDocument;
    std::string runtimeRdataDocument;
    const std::vector<std::uint8_t> asciiString = {
        'E', 'x', 'p', 'e', 'c', 't', 'e', 'd', 'S', 't', 'r', 'i', 'n', 'g' };
    if (!ExportImage(CreateImage(".rdata", asciiString, true), outputDirectory / "ascii",
            reports, globalDocument, runtimeRdataDocument)
        || reports.global.stringCount != 1
        || reports.global.referenceCount != 3
        || !Contains(globalDocument, "ExpectedString")
        || !Contains(globalDocument, "ip-relative-memory")
        || !Contains(globalDocument, "absolute-memory")
        || !Contains(globalDocument, "immediate"))
    {
        std::cerr << "ASCII/reference-kind regression failed\n";
        return 1;
    }

    if (!ExportImage(CreateImage(".pdata", asciiString, false), outputDirectory / "pdata",
            reports, globalDocument, runtimeRdataDocument)
        || reports.global.stringCount != 0)
    {
        std::cerr << "PE metadata section regression failed\n";
        return 2;
    }

    const std::vector<std::uint8_t> mixedUtf16 = {
        0x65, 0x78, 0x70, 0x00, 0x65, 0x6F, 0x77, 0x00, 0x6C, 0x6F, 0x67, 0x00 };
    if (!ExportImage(CreateImage(".rdata", mixedUtf16, false), outputDirectory / "mixed",
            reports, globalDocument, runtimeRdataDocument)
        || reports.global.stringCount != 0)
    {
        std::cerr << "mixed UTF-16 rejection regression failed\n";
        return 3;
    }

    const std::u16string wideText = u"ExpectedWide";
    std::vector<std::uint8_t> wideBytes(wideText.size() * sizeof(char16_t));
    std::memcpy(wideBytes.data(), wideText.data(), wideBytes.size());
    if (!ExportImage(CreateImage(".rdata", wideBytes, false), outputDirectory / "wide",
            reports, globalDocument, runtimeRdataDocument)
        || reports.global.stringCount != 1
        || !Contains(globalDocument, "ExpectedWide")
        || !Contains(globalDocument, "utf-16le"))
    {
        std::cerr << "ASCII UTF-16 regression failed\n";
        return 4;
    }

    if (!ExportImage(CreateRuntimeSectionImage(), outputDirectory / "runtime",
            reports, globalDocument, runtimeRdataDocument)
        || reports.global.stringCount != 2
        || reports.global.referenceCount != 2
        || reports.runtimeRdata.stringCount != 1
        || reports.runtimeRdata.referenceCount != 1
        || !Contains(globalDocument, "RuntimeRdataString")
        || !Contains(globalDocument, "WritableDataString")
        || !Contains(runtimeRdataDocument, "RuntimeRdataString")
        || Contains(runtimeRdataDocument, "WritableDataString")
        || !Contains(runtimeRdataDocument, "\"SectionFilter\": \".rdata\""))
    {
        std::cerr << "runtime .rdata report regression failed\n";
        return 5;
    }

    std::cout << "GlobalStringXrefSmoke passed\n";
    return 0;
}
