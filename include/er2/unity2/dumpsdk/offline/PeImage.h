#pragma once

#include <er2/unity2/dumpsdk/offline/BinaryStream.h>
#include <er2/unity2/dumpsdk/offline/MetadataStructs.h>

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace er2
{

class PeImage : public BinaryStream
{
public:
    PeImage() = default;

    PeImage(const uint8_t* base, size_t size);

    void Load(const uint8_t* base, size_t size);

    void LoadFromMemory(uint64_t base);

    bool LoadFromModuleSnapshot(HMODULE module, std::string& error);

    bool LoadFromModuleRange(uintptr_t moduleBase, size_t moduleSize, std::string& error);

    uint64_t ImageBase() const
    {
        return imageBase_;
    }

    bool IsMemoryLoaded() const
    {
        return memoryLoaded_;
    }

    const std::vector<PeSectionHeader>& Sections() const
    {
        return sections_;
    }

    std::vector<PeSectionHeader> ExecSections() const;
    std::vector<PeSectionHeader> DataSections() const;

    uint64_t MapVATR(uint64_t absoluteAddress) const;
    uint64_t MapRTVA(uint64_t offset) const;

    uint64_t ReadU64(uint64_t absoluteAddress) const;
    std::vector<uint8_t> ReadBytes(uint64_t absoluteAddress, size_t count) const;

    std::string ReadCString(uint64_t absoluteAddress) const;

private:
    void ParsePeHeaders();

    const PeSectionHeader* FindSectionByRva(uint32_t rva) const;
    const PeSectionHeader* FindSectionByOffset(uint32_t offset) const;

    uint64_t imageBase_ = 0;
    bool memoryLoaded_ = false;
    std::vector<PeSectionHeader> sections_;
    std::vector<uint8_t> ownedBytes_;
};

} // namespace er2
