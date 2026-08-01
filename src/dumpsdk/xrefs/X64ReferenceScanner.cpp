#include "X64ReferenceScanner.h"

#include "X64InstructionDecoder.h"
#include "XrefPeAccess.h"

#include <er2/unity2/dumpsdk/dump_progress.hpp>

#include <algorithm>

namespace er2
{

std::vector<GlobalStringReferenceCandidate> X64ReferenceScanner::Scan(const PeImage& image)
{
    std::vector<GlobalStringReferenceCandidate> result;
    std::uint64_t totalBytes = 0;
    for (const PeSectionHeader& section : image.Sections())
    {
        if ((section.characteristics & kImageScnMemExecute) != 0)
        {
            totalBytes += section.Span();
        }
    }
    DumpSdkProgressScope progress("Scan native xrefs", totalBytes);
    std::uint64_t processedBytes = 0;
    constexpr std::size_t ProgressChunkSize = 64u * 1024u;

    for (const PeSectionHeader& section : image.Sections())
    {
        const std::uint32_t sectionSize = section.Span();
        if (sectionSize == 0 || (section.characteristics & kImageScnMemExecute) == 0)
        {
            continue;
        }

        const std::uint8_t* bytes = XrefRvaToData(image, section.virtualAddress, sectionSize);
        if (bytes == nullptr)
        {
            processedBytes += sectionSize;
            progress.Update(processedBytes);
            continue;
        }

        std::size_t offset = 0;
        std::size_t nextProgressOffset = ProgressChunkSize;
        while (offset < sectionSize)
        {
            DecodedReferenceInstruction decoded;
            if (!X64InstructionDecoder::TryDecode(
                    bytes + offset,
                    sectionSize - offset,
                    static_cast<std::uintptr_t>(image.ImageBase()) + section.virtualAddress + offset,
                    decoded))
            {
                ++offset;
            }
            else if (decoded.kind != nullptr && decoded.targetAddress != 0)
            {
                result.push_back({
                    section.virtualAddress + static_cast<std::uint32_t>(offset),
                    decoded.targetAddress,
                    XrefSectionName(section),
                    decoded.kind,
                    decoded.mnemonic });
                offset += std::max<std::size_t>(decoded.length, 1);
            }
            else
            {
                ++offset;
            }

            if (offset >= nextProgressOffset || offset == sectionSize)
            {
                progress.Update(processedBytes + offset);
                nextProgressOffset = offset + ProgressChunkSize;
            }
        }
        processedBytes += sectionSize;
        progress.Update(processedBytes);
    }
    progress.Complete();
    return result;
}

} // namespace er2
