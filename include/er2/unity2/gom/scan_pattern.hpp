#pragma once

namespace er2
{

inline std::uint64_t GomScanNowMs()
{
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count());
}

inline void GomScanLog(const char* fmt, ...)
{
    std::fprintf(stderr, "[GOM_SCAN] ");
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
}

inline bool IsSectionWantedForGomScan(const char* name)
{
    if (!name || !name[0])
    {
        return false;
    }

    std::string s(name);
    for (auto& c : s)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return s == ".data" || s == ".rdata";
}

inline bool IsReadableProtectForGlobalScan(std::uint32_t protect)
{
    const std::uint32_t p = protect & 0xFFu;
    return p == PAGE_READONLY ||
        p == PAGE_READWRITE ||
        p == PAGE_WRITECOPY ||
        p == PAGE_EXECUTE_READ ||
        p == PAGE_EXECUTE_READWRITE ||
        p == PAGE_EXECUTE_WRITECOPY;
}

inline bool ScanProcessForPatternGlobal(
    const IMemoryAccessor& mem,
    HANDLE process,
    const std::uint8_t* pattern,
    std::size_t patternSize,
    std::vector<std::uintptr_t>& outHits,
    std::size_t maxHits,
    std::size_t chunkSize)
{
    if (!process || !pattern || patternSize == 0)
    {
        return false;
    }
    if (chunkSize < 0x1000)
    {
        chunkSize = 0x1000;
    }
    if (chunkSize < patternSize)
    {
        chunkSize = patternSize;
    }

    outHits.clear();

    constexpr std::uintptr_t kUserMax = 0x00007FFFFFFFFFFFull;
    MEMORY_BASIC_INFORMATION mbi{};
    std::uintptr_t cursor = 0;

    std::vector<std::uint8_t> chunk;
    std::vector<std::uint8_t> tail;

    while (cursor < kUserMax)
    {
        const SIZE_T q = VirtualQueryEx(process, reinterpret_cast<LPCVOID>(cursor), &mbi, sizeof(mbi));
        if (q == 0)
        {
            break;
        }

        const std::uintptr_t regionBase = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
        const std::uintptr_t regionSize = static_cast<std::uintptr_t>(mbi.RegionSize);
        const std::uintptr_t regionEnd = regionBase + regionSize;
        if (regionEnd <= cursor)
        {
            break;
        }
        cursor = regionEnd;

        if (regionSize < patternSize)
        {
            continue;
        }
        if (mbi.State != MEM_COMMIT)
        {
            continue;
        }
        if ((mbi.Protect & PAGE_GUARD) != 0 || (mbi.Protect & PAGE_NOACCESS) != 0)
        {
            continue;
        }
        if (!IsReadableProtectForGlobalScan(static_cast<std::uint32_t>(mbi.Protect)))
        {
            continue;
        }

        std::uintptr_t readAddr = regionBase;
        tail.clear();
        while (readAddr < regionEnd)
        {
            const std::uintptr_t remain = regionEnd - readAddr;
            const std::size_t toRead = static_cast<std::size_t>(remain > chunkSize ? chunkSize : remain);

            chunk.resize(tail.size() + toRead);
            if (!tail.empty())
            {
                std::memcpy(chunk.data(), tail.data(), tail.size());
            }

            if (!mem.Read(readAddr, chunk.data() + tail.size(), toRead))
            {
                tail.clear();
                readAddr += static_cast<std::uintptr_t>(toRead);
                continue;
            }

            const std::size_t total = tail.size() + toRead;
            if (total >= patternSize)
            {
                const std::uintptr_t blockBase = readAddr - static_cast<std::uintptr_t>(tail.size());
                const std::size_t last = total - patternSize;
                for (std::size_t i = 0; i <= last; ++i)
                {
                    if (std::memcmp(chunk.data() + i, pattern, patternSize) == 0)
                    {
                        outHits.push_back(blockBase + static_cast<std::uintptr_t>(i));
                        if (maxHits != 0 && outHits.size() >= maxHits)
                        {
                            return true;
                        }
                    }
                }
            }

            std::size_t keep = 0;
            if (patternSize > 1)
            {
                const std::size_t maxKeep = patternSize - 1;
                keep = (total > maxKeep) ? maxKeep : total;
            }
            tail.clear();
            if (keep > 0)
            {
                tail.resize(keep);
                std::memcpy(tail.data(), chunk.data() + (total - keep), keep);
            }

            readAddr += static_cast<std::uintptr_t>(toRead);
        }
    }

    return !outHits.empty();
}

template <typename T>
inline std::array<std::uint8_t, sizeof(T)> ValueBytes(T value)
{
    std::array<std::uint8_t, sizeof(T)> out{};
    std::memcpy(out.data(), &value, sizeof(T));
    return out;
}

} // namespace er2
