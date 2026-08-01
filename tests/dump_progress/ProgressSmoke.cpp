#include <er2/unity2/dumpsdk/dump_progress.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

struct RecordedEvent
{
    std::string stage;
    std::string detail;
    std::uint64_t current = 0;
    std::uint64_t total = 0;
    er2::DumpSdkProgressState state = er2::DumpSdkProgressState::Update;
};

std::vector<RecordedEvent> Events;

void Record(const er2::DumpSdkProgressEvent& event)
{
    Events.push_back({
        std::string(event.stage),
        std::string(event.detail),
        event.current,
        event.total,
        event.state });
}

void ThrowFromCallback(const er2::DumpSdkProgressEvent&)
{
    throw std::runtime_error("callback failure");
}

void TestThrottledMonotonicSequence()
{
    Events.clear();
    er2::SetDumpSdkProgressCallback(&Record);
    {
        er2::DumpSdkProgressScope progress("Scan bytes", 10000, "runtime image");
        for (std::uint64_t current = 1; current <= 10000; ++current)
        {
            progress.Update(current);
        }
        progress.Complete();
    }

    assert(Events.size() <= 103);
    assert(Events.front().state == er2::DumpSdkProgressState::Begin);
    assert(Events.front().current == 0);
    assert(Events.back().state == er2::DumpSdkProgressState::Complete);
    assert(Events.back().current == 10000);
    assert(Events.back().total == 10000);

    std::uint64_t previous = 0;
    for (const RecordedEvent& event : Events)
    {
        assert(event.stage == "Scan bytes");
        assert(event.current >= previous);
        previous = event.current;
    }
}

void TestStageResetAndFailureState()
{
    Events.clear();
    {
        er2::DumpSdkProgressScope first("First stage", 3);
        first.Update(3);
        first.Complete();
    }
    {
        er2::DumpSdkProgressScope second("Second stage", 10);
        second.Update(4);
    }

    std::size_t beginCount = 0;
    for (const RecordedEvent& event : Events)
    {
        assert(event.stage != "Overall");
        if (event.state == er2::DumpSdkProgressState::Begin)
        {
            ++beginCount;
            assert(event.current == 0);
        }
    }
    assert(beginCount == 2);
    assert(Events.back().stage == "Second stage");
    assert(Events.back().state == er2::DumpSdkProgressState::Failed);
    assert(Events.back().current == 4);
    assert(Events.back().total == 10);
}

void TestCallbackIsolationAndClear()
{
    er2::SetDumpSdkProgressCallback(&ThrowFromCallback);
    {
        er2::DumpSdkProgressScope progress("Throwing callback", 1);
        progress.Complete();
    }

    er2::SetDumpSdkProgressCallback(nullptr);
    const std::size_t eventCount = Events.size();
    {
        er2::DumpSdkProgressScope progress("Disabled", 1);
        progress.Complete();
    }
    assert(Events.size() == eventCount);
}

} // namespace

int main()
{
    TestThrottledMonotonicSequence();
    TestStageResetAndFailureState();
    TestCallbackIsolationAndClear();
    std::cout << "dump progress smoke passed\n";
    return 0;
}
