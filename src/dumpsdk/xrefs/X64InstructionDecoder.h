#pragma once

#include <cstddef>
#include <cstdint>

namespace er2
{

struct DecodedReferenceInstruction
{
    std::size_t length = 0;
    std::uintptr_t targetAddress = 0;
    const char* kind = nullptr;
    const char* mnemonic = nullptr;
};

class X64InstructionDecoder
{
public:
    static bool TryDecode(
        const std::uint8_t* bytes,
        std::size_t available,
        std::uintptr_t instructionAddress,
        DecodedReferenceInstruction& decoded);
};

} // namespace er2
