#include <er2/os/win/local_memory_accessor.hpp>
#include <er2/unity2/metadata.hpp>

#include <Windows.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

constexpr std::uint32_t DataRva = 0x2000;
constexpr std::uint32_t DataSize = 0x04000000;
constexpr std::uint32_t ImageSize = DataRva + DataSize;
constexpr std::size_t MetadataSize = 0x22000;
constexpr double MaximumElapsedMs = 750.0;

void PutI32(std::uint8_t* destination, std::size_t offset, std::int32_t value)
{
    std::memcpy(destination + offset, &value, sizeof(value));
}

void PutU32(std::uint8_t* destination, std::size_t offset, std::uint32_t value)
{
    std::memcpy(destination + offset, &value, sizeof(value));
}

void BuildMetadataHeader(std::uint8_t* metadata)
{
    constexpr int Pairs[][2] =
    {
        {0x08, 0x0C}, {0x10, 0x14}, {0x18, 0x1C}, {0x20, 0x24},
        {0x28, 0x2C}, {0x30, 0x34}, {0x38, 0x3C}, {0x40, 0x44},
        {0x48, 0x4C}, {0x50, 0x54}, {0xA0, 0xA4}, {0xA8, 0xAC},
        {0xB0, 0xB4}, {0xB8, 0xBC}
    };

    PutU32(metadata, 0, 0xFAB11BAFu);
    PutI32(metadata, 4, 29);
    for (std::size_t index = 0; index < std::size(Pairs); ++index)
    {
        const std::int32_t tableOffset = index == std::size(Pairs) - 1
            ? 0x20000
            : 0x200 + static_cast<std::int32_t>(index) * 0x1000;
        PutI32(metadata, Pairs[index][0], tableOffset);
        PutI32(metadata, Pairs[index][1], 0x1000);
    }
    PutI32(metadata, 0xAC, 0x280);
    PutI32(metadata, 0xB4, 0x400);
}

void BuildPeHeaders(std::uint8_t* image)
{
    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = 0x80;

    IMAGE_NT_HEADERS64* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    nt->FileHeader.NumberOfSections = 2;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    nt->OptionalHeader.ImageBase = reinterpret_cast<std::uint64_t>(image);
    nt->OptionalHeader.SectionAlignment = 0x1000;
    nt->OptionalHeader.FileAlignment = 0x200;
    nt->OptionalHeader.SizeOfImage = ImageSize;
    nt->OptionalHeader.SizeOfHeaders = 0x400;

    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
    std::memcpy(sections[0].Name, ".text", 5);
    sections[0].Misc.VirtualSize = 0x1000;
    sections[0].VirtualAddress = 0x1000;
    sections[0].SizeOfRawData = 0x1000;
    sections[0].Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;

    std::memcpy(sections[1].Name, ".data", 5);
    sections[1].Misc.VirtualSize = DataSize;
    sections[1].VirtualAddress = DataRva;
    sections[1].SizeOfRawData = DataSize;
    sections[1].Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
}

} // namespace

int main()
{
    std::uint8_t* metadata = static_cast<std::uint8_t*>(VirtualAlloc(
        nullptr,
        MetadataSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
    std::uint8_t* image = static_cast<std::uint8_t*>(VirtualAlloc(
        nullptr,
        ImageSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
    if (metadata == nullptr || image == nullptr)
    {
        std::cout << "allocation failed\n";
        return 1;
    }

    BuildMetadataHeader(metadata);
    BuildPeHeaders(image);
    const std::uintptr_t metadataAddress = reinterpret_cast<std::uintptr_t>(metadata);
    std::memcpy(image + DataRva + DataSize - 16, &metadataAddress, sizeof(metadataAddress));

    er2::LocalMemoryAccessor memory;
    const auto start = std::chrono::steady_clock::now();
    const er2::FoundMetadata found = er2::FindMetadataByScore(
        memory,
        reinterpret_cast<std::uintptr_t>(image),
        0x200000,
        8192,
        5.0,
        false,
        0);
    const double elapsedMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();

    std::cout << "elapsed_ms=" << elapsedMs << "\n";
    std::cout << "found=" << (found.metaBase != 0) << "\n";

    VirtualFree(image, 0, MEM_RELEASE);
    VirtualFree(metadata, 0, MEM_RELEASE);

    if (found.metaBase != metadataAddress)
    {
        return 1;
    }
    return elapsedMs <= MaximumElapsedMs ? 0 : 2;
}
