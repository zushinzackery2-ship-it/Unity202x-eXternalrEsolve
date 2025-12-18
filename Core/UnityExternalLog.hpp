#pragma once

#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <atomic>

namespace UnityExternal
{

class ExternalLogger
{
public:
    void Open(const char* path)
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (fp_)
        {
            std::fclose(fp_);
            fp_ = nullptr;
        }

        if (!path || path[0] == '\0')
        {
            enabled_.store(true, std::memory_order_release);
            return;
        }

        fp_ = std::fopen(path, "ab");
        enabled_.store(true, std::memory_order_release);
    }

    void Close()
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (fp_)
        {
            std::fclose(fp_);
            fp_ = nullptr;
        }
    }

    void SetEnabled(bool e)
    {
        std::lock_guard<std::mutex> lock(mu_);
        enabled_.store(e, std::memory_order_release);
    }

    bool IsEnabled() const
    {
        return enabled_.load(std::memory_order_acquire);
    }

    void VLogf(const char* fmt, va_list ap)
    {
        if (!enabled_.load(std::memory_order_acquire)) return;

        char buf[2048];
#if defined(_MSC_VER)
        int n = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
#else
        int n = vsnprintf(buf, sizeof(buf), fmt, ap);
#endif
        if (n <= 0) return;

        std::lock_guard<std::mutex> lock(mu_);

        std::fwrite(buf, 1, (size_t)n, stdout);
        std::fflush(stdout);

        if (fp_)
        {
            std::fwrite(buf, 1, (size_t)n, fp_);
            std::fflush(fp_);
        }
    }

private:
    mutable std::mutex mu_;
    FILE* fp_ = nullptr;
    std::atomic<bool> enabled_{true};
};

inline ExternalLogger& GetExternalLogger()
{
    static ExternalLogger g;
    return g;
}

inline void LogInit(const char* path)
{
    GetExternalLogger().Open(path);
}

inline void LogClose()
{
    GetExternalLogger().Close();
}

inline void Logf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    GetExternalLogger().VLogf(fmt, ap);
    va_end(ap);
}

}
