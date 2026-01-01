#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../mem/memory_read.hpp"
#include "../metadata/metadata_header_fields.hpp"
#include "sdk_common.hpp"
#include "sdk_metadata_helpers.hpp"
#include "sdk_strings.hpp"

namespace er2
{

class DumpSdk6TypeResolver
{
public:
    DumpSdk6TypeResolver(
        const IMemoryAccessor& mem,
        std::uintptr_t metaBase,
        const MetadataHeaderFields& header,
        const std::vector<std::uint8_t>& metaBytes,
        std::uintptr_t typesPtr,
        std::uint32_t typesCount,
        std::unordered_map<std::uint32_t, std::string> typeMap,
        std::vector<DumpSdk6GenericParamInfo> genericParams)
        : mem_(mem)
        , metaBase_(metaBase)
        , header_(header)
        , meta_(metaBytes)
        , typesPtr_(typesPtr)
        , typesCount_(typesCount)
        , typeMap_(std::move(typeMap))
        , genericParams_(std::move(genericParams))
    {
    }

    std::string DescribeFromTypeIndex(std::int32_t typeIndex)
    {
        if (typeIndex < 0 || typesPtr_ == 0 || typesCount_ == 0)
        {
            return std::string();
        }
        const std::uint32_t idx = static_cast<std::uint32_t>(typeIndex);
        if (idx >= typesCount_)
        {
            return std::string();
        }

        auto it = indexCache_.find(idx);
        if (it != indexCache_.end())
        {
            return it->second;
        }

        std::uintptr_t typePtr = 0;
        if (!ReadPtr(mem_, typesPtr_ + static_cast<std::uintptr_t>(idx) * 8u, typePtr) || typePtr == 0)
        {
            return std::string();
        }
        pointerToIndex_.try_emplace(typePtr, idx);

        std::string desc = DescribeTypePtr(typePtr, 0);
        if (!desc.empty())
        {
            indexCache_[idx] = desc;
        }
        return desc;
    }

private:
    std::string DescribeTypePtr(std::uintptr_t typePtr, int depth)
    {
        if (typePtr == 0 || depth > 8)
        {
            return std::string();
        }

        auto it = typeCache_.find(typePtr);
        if (it != typeCache_.end())
        {
            return it->second;
        }

        std::uint8_t raw[0x18] = {};
        if (!mem_.Read(typePtr, raw, sizeof(raw)))
        {
            return std::string();
        }

        std::uintptr_t dataPtr = 0;
        std::memcpy(&dataPtr, raw + 0x00, sizeof(dataPtr));

        std::uint32_t metaBits = 0;
        std::memcpy(&metaBits, raw + 0x08, sizeof(metaBits));

        const std::uint32_t typeEnum = (metaBits >> 16) & 0xFFu;
        const std::uint32_t rest = (metaBits >> 24);
        const std::uint32_t byrefFlag = (rest >> 5) & 0x1u;

        std::string base;

        switch (typeEnum)
        {
        case 0x01: base = "System.Void"; break;
        case 0x02: base = "System.Boolean"; break;
        case 0x03: base = "System.Char"; break;
        case 0x04: base = "System.SByte"; break;
        case 0x05: base = "System.Byte"; break;
        case 0x06: base = "System.Int16"; break;
        case 0x07: base = "System.UInt16"; break;
        case 0x08: base = "System.Int32"; break;
        case 0x09: base = "System.UInt32"; break;
        case 0x0A: base = "System.Int64"; break;
        case 0x0B: base = "System.UInt64"; break;
        case 0x0C: base = "System.Single"; break;
        case 0x0D: base = "System.Double"; break;
        case 0x0E: base = "System.String"; break;
        case 0x1C: base = "System.Object"; break;
        case 0x18: base = "System.IntPtr"; break;
        case 0x19: base = "System.UIntPtr"; break;
        default: break;
        }

        if (base.empty())
        {
            if (typeEnum == 0x12u || typeEnum == 0x11u)
            {
                if (metaBase_ && header_.typeDefinitionsOffset && header_.typeDefinitionsSize)
                {
                    const std::uintptr_t tdBase = metaBase_ + static_cast<std::uintptr_t>(header_.typeDefinitionsOffset);
                    const std::uintptr_t tdEnd = tdBase + static_cast<std::uintptr_t>(header_.typeDefinitionsSize);
                    if (dataPtr >= tdBase && dataPtr < tdEnd)
                    {
                        const std::uintptr_t rel = dataPtr - tdBase;
                        if ((rel % sizeof(DumpSdk6TypeDefRaw)) == 0)
                        {
                            const std::uint32_t typeDefIndex = static_cast<std::uint32_t>(rel / sizeof(DumpSdk6TypeDefRaw));
                            const std::size_t off = static_cast<std::size_t>(header_.typeDefinitionsOffset) + static_cast<std::size_t>(typeDefIndex) * sizeof(DumpSdk6TypeDefRaw);
                            if (off + sizeof(DumpSdk6TypeDefRaw) <= meta_.size())
                            {
                                const DumpSdk6TypeDefRaw* td = reinterpret_cast<const DumpSdk6TypeDefRaw*>(meta_.data() + off);
                                std::string n;
                                std::string ns;
                                (void)ReadCStringFromMetadataBytes(meta_, header_, td->nameIndex, n);
                                (void)ReadCStringFromMetadataBytes(meta_, header_, td->namespaceIndex, ns);
                                if (!n.empty())
                                {
                                    base = ns.empty() ? n : (ns + "." + n);
                                }
                            }
                        }
                    }
                }

                if (base.empty() && dataPtr)
                {
                    std::uintptr_t namePtr = 0;
                    std::uintptr_t nsPtr = 0;
                    if (ReadPtr(mem_, dataPtr + 0x10u, namePtr) && namePtr)
                    {
                        (void)ReadCString(mem_, namePtr, base, 260);
                        (void)ReadPtr(mem_, dataPtr + 0x18u, nsPtr);
                        if (nsPtr)
                        {
                            std::string ns;
                            (void)ReadCString(mem_, nsPtr, ns, 260);
                            if (!ns.empty())
                            {
                                base = ns + "." + base;
                            }
                        }
                    }
                }

                if (base.empty())
                {
                    auto it2 = pointerToIndex_.find(typePtr);
                    if (it2 != pointerToIndex_.end())
                    {
                        const std::uint32_t idx = it2->second;
                        auto it3 = typeMap_.find(idx);
                        if (it3 != typeMap_.end())
                        {
                            base = it3->second;
                        }
                    }
                }
            }
            else if (typeEnum == 0x1Du)
            {
                const std::string elem = DescribeTypePtr(dataPtr, depth + 1);
                if (!elem.empty())
                {
                    base = elem + "[]";
                }
            }
            else if (typeEnum == 0x14u)
            {
                std::uintptr_t elemPtr = 0;
                if (dataPtr)
                {
                    (void)ReadPtr(mem_, dataPtr + 0x00u, elemPtr);
                }
                std::uint8_t rank = 0;
                if (dataPtr)
                {
                    (void)mem_.Read(dataPtr + 0x08u, &rank, 1);
                }
                const std::string elem = DescribeTypePtr(elemPtr, depth + 1);
                if (!elem.empty())
                {
                    if (rank <= 1)
                    {
                        base = elem + "[]";
                    }
                    else
                    {
                        base = elem + "[" + std::string(static_cast<std::size_t>(rank - 1), ',') + "]";
                    }
                }
            }
            else if (typeEnum == 0x0Fu)
            {
                const std::string target = DescribeTypePtr(dataPtr, depth + 1);
                if (!target.empty())
                {
                    base = target + "*";
                }
            }
            else if (typeEnum == 0x10u)
            {
                const std::string target = DescribeTypePtr(dataPtr, depth + 1);
                if (!target.empty())
                {
                    base = target + "&";
                }
            }
            else if (typeEnum == 0x15u)
            {
                std::uintptr_t baseTypePtr = 0;
                std::uintptr_t instPtr = 0;
                if (dataPtr)
                {
                    (void)ReadPtr(mem_, dataPtr + 0x00u, baseTypePtr);
                    (void)ReadPtr(mem_, dataPtr + 0x08u, instPtr);
                }

                const std::string baseDesc = DescribeTypePtr(baseTypePtr, depth + 1);

                std::vector<std::string> args;
                if (instPtr)
                {
                    std::uint32_t argc = 0;
                    std::uintptr_t argvPtr = 0;
                    if (ReadValue(mem_, instPtr + 0x00u, argc) && ReadPtr(mem_, instPtr + 0x08u, argvPtr) && argvPtr && argc > 0 && argc < 32)
                    {
                        args.reserve(argc);
                        for (std::uint32_t i = 0; i < argc; ++i)
                        {
                            std::uintptr_t argPtr = 0;
                            if (!ReadPtr(mem_, argvPtr + static_cast<std::uintptr_t>(i) * 8u, argPtr) || !argPtr)
                            {
                                break;
                            }
                            const std::string d = DescribeTypePtr(argPtr, depth + 1);
                            if (!d.empty())
                            {
                                args.push_back(d);
                            }
                        }
                    }
                }

                if (!baseDesc.empty())
                {
                    if (!args.empty())
                    {
                        std::string suffix;
                        suffix.reserve(16 + args.size() * 16);
                        suffix.push_back('<');
                        for (std::size_t i = 0; i < args.size(); ++i)
                        {
                            if (i) suffix += ", ";
                            suffix += DumpSdk6StripNamespacesInType(DumpSdk6ToCsType(args[i]));
                        }
                        suffix.push_back('>');
                        base = baseDesc + suffix;
                    }
                    else
                    {
                        base = baseDesc;
                    }
                }
            }
            else if (typeEnum == 0x13u || typeEnum == 0x1Eu)
            {
                std::uint32_t paramIndex = 0xFFFFFFFFu;

                if (metaBase_ && header_.genericParametersOffset && header_.genericParametersSize && dataPtr)
                {
                    const std::uintptr_t gpBase = metaBase_ + static_cast<std::uintptr_t>(header_.genericParametersOffset);
                    const std::uintptr_t gpEnd = gpBase + static_cast<std::uintptr_t>(header_.genericParametersSize);
                    if (dataPtr >= gpBase && dataPtr < gpEnd)
                    {
                        const std::uintptr_t rel = dataPtr - gpBase;
                        if ((rel % 16u) == 0)
                        {
                            paramIndex = static_cast<std::uint32_t>(rel / 16u);
                        }
                    }
                }

                if (paramIndex == 0xFFFFFFFFu)
                {
                    if (dataPtr < 0xFFFFFFFFu)
                    {
                        paramIndex = static_cast<std::uint32_t>(dataPtr);
                    }
                }

                if (paramIndex != 0xFFFFFFFFu && paramIndex < genericParams_.size())
                {
                    base = genericParams_[paramIndex].name;
                }
                if (base.empty())
                {
                    base = "T" + std::to_string(paramIndex);
                }
            }
        }

        if (byrefFlag && !base.empty())
        {
            base += "&";
        }

        if (!base.empty())
        {
            typeCache_[typePtr] = base;
        }
        return base;
    }

private:
    const IMemoryAccessor& mem_;
    std::uintptr_t metaBase_ = 0;
    MetadataHeaderFields header_;
    const std::vector<std::uint8_t>& meta_;

    std::uintptr_t typesPtr_ = 0;
    std::uint32_t typesCount_ = 0;

    std::unordered_map<std::uintptr_t, std::string> typeCache_;
    std::unordered_map<std::uint32_t, std::string> indexCache_;
    std::unordered_map<std::uintptr_t, std::uint32_t> pointerToIndex_;

    std::unordered_map<std::uint32_t, std::string> typeMap_;
    std::vector<DumpSdk6GenericParamInfo> genericParams_;
};

} // namespace er2
