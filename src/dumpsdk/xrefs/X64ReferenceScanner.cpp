#include "X64ReferenceScanner.h"

#include "X64InstructionDecoder.h"
#include "XrefPeAccess.h"

#include <algorithm>

namespace er2
{

std::vector<GlobalStringReferenceCandidate> X64ReferenceScanner::Scan(const PeImage& image)
{
    std::vector<GlobalStringReferenceCandidate> result;
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
            continue;
        }

        std::size_t offset = 0;
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
                continue;
            }

            if (decoded.kind != nullptr && decoded.targetAddress != 0)
            {
                result.push_back({
                    section.virtualAddress + static_cast<std::uint32_t>(offset),
                    decoded.targetAddress,
                    XrefSectionName(section),
                    decoded.kind,
                    decoded.mnemonic });
                offset += std::max<std::size_t>(decoded.length, 1);
                continue;
            }
            ++offset;
        }
    }
    return result;
}

} // namespace er2
