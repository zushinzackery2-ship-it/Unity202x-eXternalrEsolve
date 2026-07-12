#include <er2/unity2/dumpsdk/offline/PeImage.h>

#include <er2/unity2/dumpsdk/offline/SafeHostMemory.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace er2
{

namespace
{

class PeFormatError : public std::runtime_error
{
public:
    explicit PeFormatError(const char* message)
        : std::runtime_error(message)
    {
    }
};

DosHeader ReadDosHeader(BinaryStream& stream)
{
    DosHeader header{};
    header.magic = stream.ReadUInt16();
    header.cblp = stream.ReadUInt16();
    header.cp = stream.ReadUInt16();
    header.crlc = stream.ReadUInt16();
    header.cparhdr = stream.ReadUInt16();
    header.minalloc = stream.ReadUInt16();
    header.maxalloc = stream.ReadUInt16();
    header.ss = stream.ReadUInt16();
    header.sp = stream.ReadUInt16();
    header.csum = stream.ReadUInt16();
    header.ip = stream.ReadUInt16();
    header.cs = stream.ReadUInt16();
    header.lfarlc = stream.ReadUInt16();
    header.ovno = stream.ReadUInt16();
    for (int i = 0; i < 4; ++i)
    {
        header.res[i] = stream.ReadUInt16();
    }
    header.oemid = stream.ReadUInt16();
    header.oeminfo = stream.ReadUInt16();
    for (int i = 0; i < 10; ++i)
    {
        header.res2[i] = stream.ReadUInt16();
    }
    header.lfanew = stream.ReadUInt32();
    return header;
}

PeFileHeader ReadFileHeader(BinaryStream& stream)
{
    PeFileHeader header{};
    header.machine = stream.ReadUInt16();
    header.numberOfSections = stream.ReadUInt16();
    header.timeDateStamp = stream.ReadUInt32();
    header.pointerToSymbolTable = stream.ReadUInt32();
    header.numberOfSymbols = stream.ReadUInt32();
    header.sizeOfOptionalHeader = stream.ReadUInt16();
    header.characteristics = stream.ReadUInt16();
    return header;
}

PeOptionalHeader64 ReadOptionalHeader64(BinaryStream& stream)
{
    PeOptionalHeader64 header{};
    header.magic = stream.ReadUInt16();
    header.majorLinkerVersion = stream.ReadUInt8();
    header.minorLinkerVersion = stream.ReadUInt8();
    header.sizeOfCode = stream.ReadUInt32();
    header.sizeOfInitializedData = stream.ReadUInt32();
    header.sizeOfUninitializedData = stream.ReadUInt32();
    header.addressOfEntryPoint = stream.ReadUInt32();
    header.baseOfCode = stream.ReadUInt32();
    header.imageBase = stream.ReadUInt64();
    header.sectionAlignment = stream.ReadUInt32();
    header.fileAlignment = stream.ReadUInt32();
    header.majorOperatingSystemVersion = stream.ReadUInt16();
    header.minorOperatingSystemVersion = stream.ReadUInt16();
    header.majorImageVersion = stream.ReadUInt16();
    header.minorImageVersion = stream.ReadUInt16();
    header.majorSubsystemVersion = stream.ReadUInt16();
    header.minorSubsystemVersion = stream.ReadUInt16();
    header.win32VersionValue = stream.ReadUInt32();
    header.sizeOfImage = stream.ReadUInt32();
    header.sizeOfHeaders = stream.ReadUInt32();
    header.checkSum = stream.ReadUInt32();
    header.subsystem = stream.ReadUInt16();
    header.dllCharacteristics = stream.ReadUInt16();
    header.sizeOfStackReserve = stream.ReadUInt64();
    header.sizeOfStackCommit = stream.ReadUInt64();
    header.sizeOfHeapReserve = stream.ReadUInt64();
    header.sizeOfHeapCommit = stream.ReadUInt64();
    header.loaderFlags = stream.ReadUInt32();
    header.numberOfRvaAndSizes = stream.ReadUInt32();
    return header;
}

PeSectionHeader ReadSectionHeader(BinaryStream& stream)
{
    PeSectionHeader header{};
    const std::vector<uint8_t> nameBytes = stream.ReadBytes(8);
    std::memcpy(header.name, nameBytes.data(), nameBytes.size());
    header.virtualSize = stream.ReadUInt32();
    header.virtualAddress = stream.ReadUInt32();
    header.sizeOfRawData = stream.ReadUInt32();
    header.pointerToRawData = stream.ReadUInt32();
    header.pointerToRelocations = stream.ReadUInt32();
    header.pointerToLinenumbers = stream.ReadUInt32();
    header.numberOfRelocations = stream.ReadUInt16();
    header.numberOfLinenumbers = stream.ReadUInt16();
    header.characteristics = stream.ReadUInt32();
    return header;
}

bool IsExecSection(uint32_t characteristics)
{
    return characteristics == 0x60000020u;
}

bool IsDataSection(uint32_t characteristics)
{
    return characteristics == 0x40000040u || characteristics == 0xC0000040u;
}

} // namespace

PeImage::PeImage(const uint8_t* base, size_t size)
{
    Load(base, size);
}

void PeImage::Load(const uint8_t* base, size_t size)
{
    Bind(base, size);
    memoryLoaded_ = false;
    sections_.clear();
    ParsePeHeaders();
}

void PeImage::ParsePeHeaders()
{
    if (Size() < sizeof(DosHeader))
    {
        throw PeFormatError("PE image too small for DOS header");
    }

    SetPosition(0);
    const DosHeader dosHeader = ReadDosHeader(*this);
    if (dosHeader.magic != 0x5A4Du)
    {
        throw PeFormatError("invalid DOS header");
    }

    SetPosition(dosHeader.lfanew);
    if (ReadUInt32() != 0x4550u)
    {
        throw PeFormatError("invalid PE signature");
    }

    const PeFileHeader fileHeader = ReadFileHeader(*this);
    const size_t optionalHeaderPos = Position();
    const uint16_t optionalMagic = ReadUInt16();
    SetPosition(optionalHeaderPos);

    PeOptionalHeader64 optionalHeader{};
    if (optionalMagic == 0x10B)
    {
        throw PeFormatError("PE32 is not supported, expected PE32+");
    }
    if (optionalMagic != 0x20B)
    {
        throw PeFormatError("invalid optional header magic");
    }

    optionalHeader = ReadOptionalHeader64(*this);
    imageBase_ = optionalHeader.imageBase;

    SetPosition(optionalHeaderPos + fileHeader.sizeOfOptionalHeader);
    sections_.clear();
    sections_.reserve(fileHeader.numberOfSections);
    for (uint16_t i = 0; i < fileHeader.numberOfSections; ++i)
    {
        sections_.push_back(ReadSectionHeader(*this));
    }
}

void PeImage::LoadFromMemory(uint64_t base)
{
    imageBase_ = base;
    memoryLoaded_ = true;
    for (PeSectionHeader& section : sections_)
    {
        section.pointerToRawData = section.virtualAddress;
        section.sizeOfRawData = section.virtualSize;
    }
}

bool PeImage::LoadFromModuleSnapshot(HMODULE module, std::string& error)
{
    uintptr_t base = 0;
    size_t size = 0;
    if (!HostMemoryTrySnapshotModule(module, ownedBytes_, base, size, error))
    {
        ownedBytes_.clear();
        return false;
    }

    try
    {
        Load(ownedBytes_.data(), ownedBytes_.size());
        LoadFromMemory(base);
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        ownedBytes_.clear();
        Bind(nullptr, 0);
        sections_.clear();
        memoryLoaded_ = false;
        imageBase_ = 0;
        return false;
    }
}

bool PeImage::LoadFromModuleRange(uintptr_t moduleBase, size_t moduleSize, std::string& error)
{
    if (!HostMemoryTrySnapshotRange(moduleBase, moduleSize, ownedBytes_, error))
    {
        ownedBytes_.clear();
        return false;
    }

    try
    {
        Load(ownedBytes_.data(), ownedBytes_.size());
        LoadFromMemory(moduleBase);
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        ownedBytes_.clear();
        Bind(nullptr, 0);
        sections_.clear();
        memoryLoaded_ = false;
        imageBase_ = 0;
        return false;
    }
}

const PeSectionHeader* PeImage::FindSectionByRva(uint32_t rva) const
{
    for (const PeSectionHeader& section : sections_)
    {
        if (section.ContainsRva(rva))
        {
            return &section;
        }
    }
    return nullptr;
}

const PeSectionHeader* PeImage::FindSectionByOffset(uint32_t offset) const
{
    for (const PeSectionHeader& section : sections_)
    {
        if (section.ContainsOffset(offset))
        {
            return &section;
        }
    }
    return nullptr;
}

uint64_t PeImage::MapVATR(uint64_t absoluteAddress) const
{
    if (!memoryLoaded_)
    {
        throw PeFormatError("LoadFromMemory must be called before MapVATR");
    }
    if (absoluteAddress < imageBase_)
    {
        return 0;
    }

    const uint32_t rva = static_cast<uint32_t>(absoluteAddress - imageBase_);
    const PeSectionHeader* section = FindSectionByRva(rva);
    if (section == nullptr)
    {
        return 0;
    }

    return static_cast<uint64_t>(rva - section->virtualAddress + section->pointerToRawData);
}

uint64_t PeImage::MapRTVA(uint64_t offset) const
{
    if (!memoryLoaded_)
    {
        throw PeFormatError("LoadFromMemory must be called before MapRTVA");
    }

    const uint32_t fileOffset = static_cast<uint32_t>(offset);
    const PeSectionHeader* section = FindSectionByOffset(fileOffset);
    if (section == nullptr)
    {
        return 0;
    }

    return static_cast<uint64_t>(fileOffset - section->pointerToRawData + section->virtualAddress) + imageBase_;
}

uint64_t PeImage::ReadU64(uint64_t absoluteAddress) const
{
    const uint64_t offset = MapVATR(absoluteAddress);
    if (offset == 0 && absoluteAddress != imageBase_)
    {
        throw StreamBoundsError("MapVATR failed for ReadU64");
    }

    PeImage reader(Data(), Size());
    reader.sections_ = sections_;
    reader.imageBase_ = imageBase_;
    reader.memoryLoaded_ = memoryLoaded_;
    reader.SetPosition(static_cast<size_t>(offset));
    return reader.ReadUInt64();
}

std::vector<uint8_t> PeImage::ReadBytes(uint64_t absoluteAddress, size_t count) const
{
    const uint64_t offset = MapVATR(absoluteAddress);
    if (offset == 0 && absoluteAddress != imageBase_)
    {
        throw StreamBoundsError("MapVATR failed for ReadBytes");
    }

    PeImage reader(Data(), Size());
    reader.sections_ = sections_;
    reader.imageBase_ = imageBase_;
    reader.memoryLoaded_ = memoryLoaded_;
    reader.SetPosition(static_cast<size_t>(offset));
    return reader.BinaryStream::ReadBytes(count);
}

std::string PeImage::ReadCString(uint64_t absoluteAddress) const
{
    const uint64_t offset = MapVATR(absoluteAddress);
    if (offset == 0 && absoluteAddress != imageBase_)
    {
        throw StreamBoundsError("MapVATR failed for ReadCString");
    }

    PeImage reader(Data(), Size());
    reader.SetPosition(static_cast<size_t>(offset));
    return reader.ReadStringToNull();
}

std::vector<PeSectionHeader> PeImage::ExecSections() const
{
    std::vector<PeSectionHeader> result;
    for (const PeSectionHeader& section : sections_)
    {
        if (IsExecSection(section.characteristics))
        {
            result.push_back(section);
        }
    }
    return result;
}

std::vector<PeSectionHeader> PeImage::DataSections() const
{
    std::vector<PeSectionHeader> result;
    for (const PeSectionHeader& section : sections_)
    {
        if (IsDataSection(section.characteristics))
        {
            result.push_back(section);
        }
    }
    return result;
}

} // namespace er2
