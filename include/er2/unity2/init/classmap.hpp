#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "context.hpp"

#include "../metadata/pe.hpp"
#include "../dumpsdk/sdk_common.hpp"
#include "../dumpsdk/sdk_metadata_helpers.hpp"

#include "../metadata/export.hpp"
#include "../metadata/hint/hint_export.hpp"
#include "../metadata/header/metadata_header_fields.hpp"

namespace er2
{

inline bool IsValidRemotePtrRange(std::uintptr_t p)
{
    return p > 0x10000 && p < 0x7FFFFFFFFFFF;
}

inline bool FindGameAssemblyDataSection(std::uintptr_t base, std::uintptr_t size, std::uintptr_t& outBase, std::uintptr_t& outSize)
{
    (void)size;
    const IMemoryAccessor& mem = Mem();

    std::uint32_t sizeOfImage = 0;
    std::vector<ModuleSection> sections;
    if (!ReadModuleSections(mem, base, sizeOfImage, sections))
    {
        return false;
    }

    for (const auto& sec : sections)
    {
        if (sec.name == ".data")
        {
            outBase = base + static_cast<std::uintptr_t>(sec.rva);
            outSize = static_cast<std::uintptr_t>(sec.size);
            return true;
        }
    }

    return false;
}

inline bool ReadIl2CppClassFullNameForClassMap(const IMemoryAccessor& mem, const Offsets& offRef, std::uintptr_t klass, std::string& outFullName)
{
    outFullName.clear();

    std::uintptr_t namePtr = 0;
    if (!ReadPtr(mem, klass + static_cast<std::uintptr_t>(offRef.il2cppclass_name_ptr), namePtr) || !namePtr)
    {
        return false;
    }

    std::string name;
    if (!ReadCString(mem, namePtr, name, 128) || name.empty())
    {
        return false;
    }

    std::uintptr_t nsPtr = 0;
    std::string ns;
    if (ReadPtr(mem, klass + static_cast<std::uintptr_t>(offRef.il2cppclass_namespace_ptr), nsPtr) && nsPtr)
    {
        (void)ReadCString(mem, nsPtr, ns, 256);
    }

    outFullName = ns.empty() ? name : (ns + "." + name);
    return true;
}

inline bool IsTypeInfoTableCandidate(const IMemoryAccessor& mem, std::uintptr_t tablePtr, const Offsets& offRef, const std::vector<std::uint32_t>& sampleByvals, const std::vector<std::string>& sampleNames)
{
    if (!IsValidRemotePtrRange(tablePtr))
    {
        return false;
    }
    if ((tablePtr & 0xFu) != 0)
    {
        return false;
    }

    std::uint32_t nonZeroCount = 0;
    std::uint32_t matchedCount = 0;
    bool hasStrongAnchor = false;
    for (std::size_t si = 0; si < sampleByvals.size(); ++si)
    {
        const std::uint32_t byval = sampleByvals[si];
        const bool isStrongAnchor =
            (sampleNames[si] == "System.Object") ||
            (sampleNames[si] == "System.String") ||
            (sampleNames[si] == "UnityEngine.Object") ||
            (sampleNames[si] == "UnityEngine.Transform");
        std::uintptr_t entry = 0;
        if (!mem.Read(tablePtr + static_cast<std::uintptr_t>(byval) * 8u, &entry, sizeof(entry)))
        {
            return false;
        }

        if (entry == 0)
        {
            continue;
        }

        ++nonZeroCount;
        if (isStrongAnchor)
        {
            hasStrongAnchor = true;
        }
        if (!IsValidRemotePtrRange(entry))
        {
            return false;
        }

        std::string fullName;
        if (!ReadIl2CppClassFullNameForClassMap(mem, offRef, entry, fullName))
        {
            return false;
        }

        if (fullName != sampleNames[si])
        {
            return false;
        }

        ++matchedCount;
    }
    return nonZeroCount >= 2 && matchedCount >= 2 && hasStrongAnchor;
}

inline bool InitIl2CppTypeInfoInternal()
{
    if (!IsInited())
    {
        return false;
    }
    if (Runtime() != ManagedBackend::Il2Cpp)
    {
        return false;
    }

    if (g_ctx.typeInfoTable && !g_ctx.byvalToFullName.empty())
    {
        return true;
    }

    if (!g_ctx.gameAssembly.base)
    {
        ModuleInfo ga;
        if (!GetContextModuleInfo(g_ctx.pid, L"GameAssembly.dll", ga) || !ga.base)
        {
            return false;
        }
        g_ctx.gameAssembly = ga;
    }
    if (!g_ctx.gameAssembly.size)
    {
        g_ctx.gameAssembly.size = 0x20000000u;
    }

    const IMemoryAccessor& mem = Mem();

    MetadataHint hint;
    if (!BuildMetadataHintTScore(mem, g_ctx.gameAssembly.base, g_ctx.pid, L"", L"GameAssembly.dll", hint))
    {
        return false;
    }

    std::vector<std::uint8_t> metaBytes;
    if (!ExportMetadataByScore(mem, g_ctx.gameAssembly.base, 0x200000u, 8192, 15.0, false, 0, 0x200000u, metaBytes))
    {
        return false;
    }

    MetadataHeaderFields header;
    if (!ReadMetadataHeaderFieldsFromBytes(metaBytes, header))
    {
        return false;
    }

    std::vector<std::string> typeFullName;
    std::unordered_map<std::uint32_t, std::string> typeMap;
    if (!BuildTypeFullNameAndByvalMapFromBytes(metaBytes, header, typeFullName, typeMap))
    {
        return false;
    }

    std::uint32_t typesCount = 0;
    for (const auto& kv : typeMap)
    {
        std::uint32_t idx = kv.first + 1u;
        if (idx > typesCount)
        {
            typesCount = idx;
        }
    }

    if (typesCount == 0)
    {
        return false;
    }

    g_ctx.byvalToFullName = typeMap;
    g_ctx.fullNameToByval.clear();
    g_ctx.fullNameToByval.reserve(typeMap.size());
    for (const auto& kv : typeMap)
    {
        g_ctx.fullNameToByval.emplace(kv.second, kv.first);
    }
    g_ctx.typeInfoCount = typesCount;


    std::vector<std::uint32_t> sampleByvals;
    std::vector<std::string> sampleNames;
    sampleByvals.reserve(8);
    sampleNames.reserve(8);
    std::unordered_set<std::uint32_t> sampledByvals;
    sampledByvals.reserve(8);
    const auto addSampleByName = [&](const char* fullName) -> void
    {
        auto it = g_ctx.fullNameToByval.find(fullName);
        if (it == g_ctx.fullNameToByval.end())
        {
            return;
        }
        if (!sampledByvals.insert(it->second).second)
        {
            return;
        }
        sampleByvals.push_back(it->second);
        sampleNames.push_back(it->first);
    };

    addSampleByName("System.Object");
    addSampleByName("System.String");
    addSampleByName("UnityEngine.Object");
    addSampleByName("UnityEngine.Transform");

    std::vector<std::uint32_t> allByvals;
    allByvals.reserve(typeMap.size());
    for (const auto& kv : typeMap)
    {
        allByvals.push_back(kv.first);
    }
    std::sort(allByvals.begin(), allByvals.end());

    const std::size_t maxSamples = 8u;
    if (!allByvals.empty() && sampleByvals.size() < maxSamples)
    {
        const std::size_t remain = maxSamples - sampleByvals.size();
        const std::size_t desired = (allByvals.size() < remain) ? allByvals.size() : remain;
        for (std::size_t i = 0; i < desired; ++i)
        {
            const std::size_t idx = (desired == 1) ? (allByvals.size() / 2) : ((i * (allByvals.size() - 1)) / (desired - 1));

            std::uint32_t byval = allByvals[idx];
            if (!sampledByvals.insert(byval).second)
            {
                continue;
            }

            auto it = typeMap.find(byval);
            if (it == typeMap.end())
            {
                continue;
            }

            sampleByvals.push_back(byval);
            sampleNames.push_back(it->second);
            if (sampleByvals.size() >= maxSamples)
            {
                break;
            }
        }
    }

    if (sampleByvals.empty())
    {
        return false;
    }

    std::uintptr_t dataBase = g_ctx.gameAssembly.base;
    std::uintptr_t dataSize = g_ctx.gameAssembly.size;
    std::uintptr_t secBase = 0;
    std::uintptr_t secSize = 0;
    if (FindGameAssemblyDataSection(dataBase, dataSize, secBase, secSize))
    {
        dataBase = secBase;
        dataSize = secSize;
    }

    const Offsets& offRef = Off();
    const auto findTypeInfoTableInRange = [&](std::uintptr_t scanBegin, std::uintptr_t scanEnd) -> std::uintptr_t
    {
        if (scanBegin >= scanEnd)
        {
            return 0;
        }

        const std::size_t chunk = 0x200000u;
        std::vector<std::uint8_t> buf(chunk);
        for (std::uintptr_t cur = scanBegin; cur < scanEnd; cur += chunk)
        {
            const std::size_t sz = (cur + chunk <= scanEnd) ? chunk : (scanEnd - cur);
            if (mem.Read(cur, buf.data(), sz))
            {
                for (std::size_t off = 0; off + sizeof(std::uintptr_t) <= sz; off += sizeof(std::uintptr_t))
                {
                    std::uintptr_t ptr = 0;
                    std::memcpy(&ptr, buf.data() + off, sizeof(ptr));
                    if (IsTypeInfoTableCandidate(mem, ptr, offRef, sampleByvals, sampleNames))
                    {
                        return ptr;
                    }
                }
                continue;
            }

            // ReadProcessMemory 需要整块可读，块读失败时回退逐指针读取避免漏扫。
            for (std::uintptr_t addr = cur; addr + sizeof(std::uintptr_t) <= cur + sz; addr += sizeof(std::uintptr_t))
            {
                std::uintptr_t ptr = 0;
                if (!mem.Read(addr, &ptr, sizeof(ptr)))
                {
                    continue;
                }
                if (IsTypeInfoTableCandidate(mem, ptr, offRef, sampleByvals, sampleNames))
                {
                    return ptr;
                }
            }
        }
        return 0;
    };

    std::uintptr_t foundTable = findTypeInfoTableInRange(dataBase, dataBase + dataSize);
    if (!foundTable)
    {
        // 某些版本 table 不在 .data，回退到 GameAssembly 全模块扫描。
        foundTable = findTypeInfoTableInRange(g_ctx.gameAssembly.base, g_ctx.gameAssembly.base + g_ctx.gameAssembly.size);
    }

    if (!foundTable)
    {
        return false;
    }

    g_ctx.typeInfoTable = foundTable;
    return true;
}

inline bool EnsureIl2CppTypeInfoInited()
{
    if (!IsInited())
    {
        return false;
    }
    if (Runtime() != ManagedBackend::Il2Cpp)
    {
        return false;
    }
    if (g_ctx.typeInfoTable && !g_ctx.byvalToFullName.empty())
    {
        return true;
    }
    return InitIl2CppTypeInfoInternal();
}

inline std::int32_t FindClassIndex(const std::string& fullName)
{
    if (!EnsureIl2CppTypeInfoInited())
    {
        return -1;
    }

    auto it = g_ctx.fullNameToByval.find(fullName);
    if (it == g_ctx.fullNameToByval.end())
    {
        return -1;
    }

    return static_cast<std::int32_t>(it->second);
}

inline std::uintptr_t FindClassByIndex(std::int32_t idx)
{
    if (!EnsureIl2CppTypeInfoInited())
    {
        return 0;
    }

    if (idx < 0)
    {
        return 0;
    }

    std::uint32_t uidx = static_cast<std::uint32_t>(idx);
    if (uidx >= g_ctx.typeInfoCount)
    {
        return 0;
    }

    const IMemoryAccessor& mem = Mem();

    std::uintptr_t entry = 0;
    if (!mem.Read(g_ctx.typeInfoTable + static_cast<std::uintptr_t>(uidx) * 8u, &entry, sizeof(entry)))
    {
        return 0;
    }

    if (!IsValidRemotePtrRange(entry))
    {
        return 0;
    }

    return entry;
}

inline std::uintptr_t FindClass(const std::string& fullName)
{
    const std::int32_t idx = FindClassIndex(fullName);
    if (idx < 0)
    {
        return 0;
    }

    return FindClassByIndex(idx);
}

} // namespace er2

