#pragma once

#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefTypes.h>

#include <cstddef>
#include <cstdint>

namespace er2
{

class GlobalStringDecoder
{
public:
    static bool TryDecode(
        const std::uint8_t* bytes,
        std::size_t available,
        const GlobalStringXrefOptions& options,
        DecodedGlobalString& decoded);

private:
    static bool TryDecodeUtf8(
        const std::uint8_t* bytes,
        std::size_t available,
        const GlobalStringXrefOptions& options,
        DecodedGlobalString& decoded);
    static bool TryDecodeUtf16(
        const std::uint8_t* bytes,
        std::size_t available,
        const GlobalStringXrefOptions& options,
        DecodedGlobalString& decoded);
};

} // namespace er2
