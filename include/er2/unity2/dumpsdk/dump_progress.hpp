#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string_view>

namespace er2
{

enum class DumpSdkProgressState
{
    Begin,
    Update,
    Complete,
    Failed,
};

struct DumpSdkProgressEvent
{
    std::string_view stage;
    std::string_view detail;
    std::uint64_t current = 0;
    std::uint64_t total = 0;
    DumpSdkProgressState state = DumpSdkProgressState::Update;
};

using DumpSdkProgressCallback = void (*)(const DumpSdkProgressEvent& event);

inline std::atomic<DumpSdkProgressCallback>& DumpSdkProgressCallbackSlot() noexcept
{
    static std::atomic<DumpSdkProgressCallback> callback{ nullptr };
    return callback;
}

inline void SetDumpSdkProgressCallback(DumpSdkProgressCallback callback) noexcept
{
    DumpSdkProgressCallbackSlot().store(callback, std::memory_order_release);
}

inline void ReportDumpSdkProgress(const DumpSdkProgressEvent& event) noexcept
{
    const DumpSdkProgressCallback callback =
        DumpSdkProgressCallbackSlot().load(std::memory_order_acquire);
    if (callback == nullptr)
    {
        return;
    }

    try
    {
        callback(event);
    }
    catch (...)
    {
    }
}

class DumpSdkProgressScope
{
public:
    DumpSdkProgressScope(
        std::string_view stage,
        std::uint64_t total,
        std::string_view detail = {}) noexcept
        : stage_(stage),
          detail_(detail),
          total_(total),
          lastEmission_(Clock::now())
    {
        Emit(DumpSdkProgressState::Begin, detail_);
    }

    ~DumpSdkProgressScope()
    {
        if (active_)
        {
            Emit(DumpSdkProgressState::Failed, detail_);
        }
    }

    DumpSdkProgressScope(const DumpSdkProgressScope&) = delete;
    DumpSdkProgressScope& operator=(const DumpSdkProgressScope&) = delete;
    DumpSdkProgressScope(DumpSdkProgressScope&&) = delete;
    DumpSdkProgressScope& operator=(DumpSdkProgressScope&&) = delete;

    void Update(std::uint64_t current, std::string_view detail = {}) noexcept
    {
        if (!active_ || total_ == 0)
        {
            return;
        }

        if (current > total_)
        {
            current = total_;
        }
        if (current <= current_)
        {
            return;
        }
        current_ = current;

        const Clock::time_point now = Clock::now();
        const std::uint64_t step = total_ / 100 + (total_ % 100 == 0 ? 0 : 1);
        const bool reachedStep = current_ - lastReported_ >= step;
        const bool reachedInterval = now - lastEmission_ >= std::chrono::milliseconds(100);
        if (!reachedStep && !reachedInterval && current_ != total_)
        {
            return;
        }

        lastReported_ = current_;
        lastEmission_ = now;
        Emit(DumpSdkProgressState::Update, detail.empty() ? detail_ : detail);
    }

    void Complete(std::string_view detail = {}) noexcept
    {
        if (!active_)
        {
            return;
        }
        current_ = total_;
        Emit(DumpSdkProgressState::Complete, detail.empty() ? detail_ : detail);
        active_ = false;
    }

    void Fail(std::string_view detail = {}) noexcept
    {
        if (!active_)
        {
            return;
        }
        Emit(DumpSdkProgressState::Failed, detail.empty() ? detail_ : detail);
        active_ = false;
    }

    std::uint64_t Current() const noexcept
    {
        return current_;
    }

    std::uint64_t Total() const noexcept
    {
        return total_;
    }

private:
    using Clock = std::chrono::steady_clock;

    void Emit(DumpSdkProgressState state, std::string_view detail) const noexcept
    {
        ReportDumpSdkProgress({ stage_, detail, current_, total_, state });
    }

    std::string_view stage_;
    std::string_view detail_;
    std::uint64_t total_ = 0;
    std::uint64_t current_ = 0;
    std::uint64_t lastReported_ = 0;
    Clock::time_point lastEmission_;
    bool active_ = true;
};

} // namespace er2
