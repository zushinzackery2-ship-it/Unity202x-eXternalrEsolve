#include "SmokeChecks.h"

#include <format>
#include <fstream>
#include <iostream>
#include <sstream>

namespace OfflineBehavior
{

void Checks::Fail(const std::string& what, const std::string& detail)
{
    failures_.push_back(what + "\n      " + detail);
    std::cout << "  [FAIL] " << what << "\n      " << detail << "\n";
}

void Checks::Ok(bool condition, const std::string& what)
{
    if (condition)
    {
        ++passed_;
        return;
    }
    Fail(what, "condition was false");
}

void Checks::Equal(const std::string& actual, const std::string& expected, const std::string& what)
{
    if (actual == expected)
    {
        ++passed_;
        return;
    }
    Fail(what, "expected \"" + expected + "\" but got \"" + actual + "\"");
}

void Checks::Equal(uint64_t actual, uint64_t expected, const std::string& what)
{
    if (actual == expected)
    {
        ++passed_;
        return;
    }
    Fail(what, std::format(
        "expected {} (0x{:X}) but got {} (0x{:X})",
        expected,
        expected,
        actual,
        actual));
}

void Checks::Contains(const std::string& haystack, const std::string& needle, const std::string& what)
{
    if (haystack.find(needle) != std::string::npos)
    {
        ++passed_;
        return;
    }
    Fail(what, "missing: " + needle);
}

void Checks::NotContains(const std::string& haystack, const std::string& needle, const std::string& what)
{
    if (haystack.find(needle) == std::string::npos)
    {
        ++passed_;
        return;
    }
    Fail(what, "unexpectedly present: " + needle);
}

int Checks::Report() const
{
    std::cout << "\n" << passed_ << " passed, " << failures_.size() << " failed\n";
    if (failures_.empty())
    {
        return 0;
    }
    std::cout << "\nFailures:\n";
    for (const std::string& failure : failures_)
    {
        std::cout << "  - " << failure << "\n";
    }
    return 1;
}

std::string ReadTextFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return {};
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace OfflineBehavior
