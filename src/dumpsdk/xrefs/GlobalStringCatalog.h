#pragma once

#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefTypes.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace er2
{

class GlobalStringCatalog
{
public:
    static std::vector<DetectedGlobalString> Extract(
        const PeImage& image,
        const GlobalStringXrefOptions& options);
    static bool TryDecodeAtAddress(
        const PeImage& image,
        std::uintptr_t address,
        const GlobalStringXrefOptions& options,
        DetectedGlobalString& detected);
    static bool FindContaining(
        const std::vector<DetectedGlobalString>& strings,
        std::uintptr_t address,
        std::size_t& index);

private:
    static bool IsStringSection(const PeSectionHeader& section);
    static const PeSectionHeader* FindStringSection(
        const PeImage& image,
        std::uintptr_t address);
};

} // namespace er2
