#pragma once

#include <algorithm>
#include <cstdint>

namespace er2
{

enum PeSectionCharacteristics : uint32_t
{
    kImageScnCntCode = 0x00000020u,
    kImageScnCntInitializedData = 0x00000040u,
    kImageScnCntUninitializedData = 0x00000080u,
    kImageScnMemExecute = 0x20000000u,
    kImageScnMemRead = 0x40000000u,
    kImageScnMemWrite = 0x80000000u,
};

struct DosHeader
{
    uint16_t magic = 0;
    uint16_t cblp = 0;
    uint16_t cp = 0;
    uint16_t crlc = 0;
    uint16_t cparhdr = 0;
    uint16_t minalloc = 0;
    uint16_t maxalloc = 0;
    uint16_t ss = 0;
    uint16_t sp = 0;
    uint16_t csum = 0;
    uint16_t ip = 0;
    uint16_t cs = 0;
    uint16_t lfarlc = 0;
    uint16_t ovno = 0;
    uint16_t res[4] = {};
    uint16_t oemid = 0;
    uint16_t oeminfo = 0;
    uint16_t res2[10] = {};
    uint32_t lfanew = 0;
};

struct PeFileHeader
{
    uint16_t machine = 0;
    uint16_t numberOfSections = 0;
    uint32_t timeDateStamp = 0;
    uint32_t pointerToSymbolTable = 0;
    uint32_t numberOfSymbols = 0;
    uint16_t sizeOfOptionalHeader = 0;
    uint16_t characteristics = 0;
};

struct PeOptionalHeader64
{
    uint16_t magic = 0;
    uint8_t majorLinkerVersion = 0;
    uint8_t minorLinkerVersion = 0;
    uint32_t sizeOfCode = 0;
    uint32_t sizeOfInitializedData = 0;
    uint32_t sizeOfUninitializedData = 0;
    uint32_t addressOfEntryPoint = 0;
    uint32_t baseOfCode = 0;
    uint64_t imageBase = 0;
    uint32_t sectionAlignment = 0;
    uint32_t fileAlignment = 0;
    uint16_t majorOperatingSystemVersion = 0;
    uint16_t minorOperatingSystemVersion = 0;
    uint16_t majorImageVersion = 0;
    uint16_t minorImageVersion = 0;
    uint16_t majorSubsystemVersion = 0;
    uint16_t minorSubsystemVersion = 0;
    uint32_t win32VersionValue = 0;
    uint32_t sizeOfImage = 0;
    uint32_t sizeOfHeaders = 0;
    uint32_t checkSum = 0;
    uint16_t subsystem = 0;
    uint16_t dllCharacteristics = 0;
    uint64_t sizeOfStackReserve = 0;
    uint64_t sizeOfStackCommit = 0;
    uint64_t sizeOfHeapReserve = 0;
    uint64_t sizeOfHeapCommit = 0;
    uint32_t loaderFlags = 0;
    uint32_t numberOfRvaAndSizes = 0;
};

struct PeSectionHeader
{
    char name[9] = {};
    uint32_t virtualSize = 0;
    uint32_t virtualAddress = 0;
    uint32_t sizeOfRawData = 0;
    uint32_t pointerToRawData = 0;
    uint32_t pointerToRelocations = 0;
    uint32_t pointerToLinenumbers = 0;
    uint16_t numberOfRelocations = 0;
    uint16_t numberOfLinenumbers = 0;
    uint32_t characteristics = 0;

    uint32_t Span() const
    {
        return (std::max)(virtualSize, sizeOfRawData);
    }

    bool ContainsRva(uint32_t rva) const
    {
        return rva >= virtualAddress && rva < virtualAddress + Span();
    }

    bool ContainsOffset(uint32_t offset) const
    {
        return offset >= pointerToRawData && offset < pointerToRawData + sizeOfRawData;
    }
};

} // namespace er2
