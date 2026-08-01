#include "X64InstructionDecoder.h"

#include <cstring>

namespace er2
{

namespace
{

struct OpcodeInfo
{
    bool valid = false;
    bool hasModRm = false;
    std::size_t immediateSize = 0;
    const char* mnemonic = "unknown";
};

bool IsLegacyPrefix(std::uint8_t value)
{
    switch (value)
    {
    case 0x26:
    case 0x2E:
    case 0x36:
    case 0x3E:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0xF0:
    case 0xF2:
    case 0xF3:
        return true;
    default:
        return false;
    }
}

OpcodeInfo GetOneByteOpcode(std::uint8_t opcode, bool operand16)
{
    if (opcode <= 0x03 || (opcode >= 0x08 && opcode <= 0x3B && (opcode & 0x04) == 0))
    {
        static const char* names[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
        return { true, true, 0, names[opcode >> 3] };
    }

    switch (opcode)
    {
    case 0x63: return { true, true, 0, "movsxd" };
    case 0x69: return { true, true, operand16 ? 2u : 4u, "imul" };
    case 0x6B: return { true, true, 1, "imul" };
    case 0x80:
    case 0x82:
    case 0x83: return { true, true, 1, "alu" };
    case 0x81: return { true, true, operand16 ? 2u : 4u, "alu" };
    case 0x84:
    case 0x85: return { true, true, 0, "test" };
    case 0x86:
    case 0x87: return { true, true, 0, "xchg" };
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B: return { true, true, 0, "mov" };
    case 0x8D: return { true, true, 0, "lea" };
    case 0x8F: return { true, true, 0, "pop" };
    case 0xC0:
    case 0xC1: return { true, true, 1, "shift" };
    case 0xC6: return { true, true, 1, "mov" };
    case 0xC7: return { true, true, operand16 ? 2u : 4u, "mov" };
    case 0xD0:
    case 0xD1:
    case 0xD2:
    case 0xD3: return { true, true, 0, "shift" };
    case 0xF6:
    case 0xF7: return { true, true, 0, "test" };
    case 0xFE: return { true, true, 0, "incdec" };
    case 0xFF: return { true, true, 0, "group5" };
    default: return {};
    }
}

OpcodeInfo GetTwoByteOpcode(std::uint8_t opcode)
{
    if ((opcode >= 0x10 && opcode <= 0x17)
        || (opcode >= 0x28 && opcode <= 0x2F)
        || (opcode >= 0x40 && opcode <= 0x4F)
        || (opcode >= 0x90 && opcode <= 0x9F)
        || (opcode >= 0xB0 && opcode <= 0xB7)
        || (opcode >= 0xBC && opcode <= 0xBF))
    {
        return { true, true, 0, "extended" };
    }

    switch (opcode)
    {
    case 0x6E:
    case 0x6F:
    case 0x7E:
    case 0x7F: return { true, true, 0, "mov" };
    case 0xA3: return { true, true, 0, "bt" };
    case 0xAB: return { true, true, 0, "bts" };
    case 0xAF: return { true, true, 0, "imul" };
    case 0xBA: return { true, true, 1, "bit" };
    default: return {};
    }
}

std::int32_t ReadInt32(const std::uint8_t* bytes)
{
    std::int32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

std::uint64_t ReadUInt64(const std::uint8_t* bytes)
{
    std::uint64_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

} // namespace

bool X64InstructionDecoder::TryDecode(
    const std::uint8_t* bytes,
    std::size_t available,
    std::uintptr_t instructionAddress,
    DecodedReferenceInstruction& decoded)
{
    decoded = {};
    std::size_t cursor = 0;
    bool operand16 = false;
    bool address32 = false;
    bool rexW = false;
    while (cursor < available && cursor < 14)
    {
        const std::uint8_t value = bytes[cursor];
        if (IsLegacyPrefix(value))
        {
            operand16 = operand16 || value == 0x66;
            address32 = address32 || value == 0x67;
            ++cursor;
            continue;
        }
        if (value >= 0x40 && value <= 0x4F)
        {
            rexW = (value & 0x08) != 0;
            ++cursor;
            continue;
        }
        break;
    }
    if (cursor >= available)
    {
        return false;
    }

    const std::uint8_t firstOpcode = bytes[cursor++];
    if (firstOpcode >= 0xB8 && firstOpcode <= 0xBF && rexW)
    {
        if (available - cursor < sizeof(std::uint64_t))
        {
            return false;
        }
        decoded.length = cursor + sizeof(std::uint64_t);
        decoded.targetAddress = static_cast<std::uintptr_t>(ReadUInt64(bytes + cursor));
        decoded.kind = "immediate";
        decoded.mnemonic = "mov";
        return true;
    }
    if (firstOpcode >= 0xA0 && firstOpcode <= 0xA3)
    {
        const std::size_t addressSize = address32 ? 4 : 8;
        if (available - cursor < addressSize)
        {
            return false;
        }
        decoded.length = cursor + addressSize;
        decoded.targetAddress = addressSize == 8
            ? static_cast<std::uintptr_t>(ReadUInt64(bytes + cursor))
            : static_cast<std::uint32_t>(ReadInt32(bytes + cursor));
        decoded.kind = "absolute-memory";
        decoded.mnemonic = "mov";
        return true;
    }

    OpcodeInfo opcodeInfo;
    if (firstOpcode == 0x0F)
    {
        if (cursor >= available)
        {
            return false;
        }
        opcodeInfo = GetTwoByteOpcode(bytes[cursor++]);
    }
    else
    {
        opcodeInfo = GetOneByteOpcode(firstOpcode, operand16);
    }
    if (!opcodeInfo.valid || !opcodeInfo.hasModRm || cursor >= available)
    {
        return false;
    }

    const std::uint8_t modRm = bytes[cursor++];
    const std::uint8_t mod = modRm >> 6;
    const std::uint8_t reg = (modRm >> 3) & 7;
    const std::uint8_t rm = modRm & 7;
    if (firstOpcode == 0xF6 && reg <= 1)
    {
        opcodeInfo.immediateSize = 1;
    }
    else if (firstOpcode == 0xF7 && reg <= 1)
    {
        opcodeInfo.immediateSize = operand16 ? 2 : 4;
    }
    else if (firstOpcode == 0xFF)
    {
        opcodeInfo.mnemonic = reg == 2 ? "call" : reg == 4 ? "jmp" : reg == 6 ? "push" : "group5";
    }

    bool ripRelative = false;
    bool absoluteMemory = false;
    std::int32_t displacement = 0;
    std::size_t displacementSize = 0;
    if (mod != 3)
    {
        if (rm == 4)
        {
            if (cursor >= available)
            {
                return false;
            }
            const std::uint8_t sib = bytes[cursor++];
            if (mod == 0 && (sib & 7) == 5)
            {
                absoluteMemory = true;
                displacementSize = 4;
            }
        }
        else if (mod == 0 && rm == 5)
        {
            ripRelative = !address32;
            absoluteMemory = address32;
            displacementSize = 4;
        }

        if (mod == 1)
        {
            displacementSize = 1;
        }
        else if (mod == 2)
        {
            displacementSize = 4;
        }
    }

    if (available - cursor < displacementSize + opcodeInfo.immediateSize)
    {
        return false;
    }
    if (displacementSize == 4)
    {
        displacement = ReadInt32(bytes + cursor);
    }
    cursor += displacementSize + opcodeInfo.immediateSize;
    if (cursor == 0 || cursor > 15)
    {
        return false;
    }

    decoded.length = cursor;
    decoded.mnemonic = opcodeInfo.mnemonic;
    if (ripRelative)
    {
        decoded.targetAddress = static_cast<std::uintptr_t>(
            static_cast<std::int64_t>(instructionAddress + decoded.length) + displacement);
        decoded.kind = "ip-relative-memory";
    }
    else if (absoluteMemory)
    {
        decoded.targetAddress = static_cast<std::uint32_t>(displacement);
        decoded.kind = "absolute-memory";
    }
    return true;
}

} // namespace er2
