#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace OfflineBehavior
{

class Checks
{
public:
    void Ok(bool condition, const std::string& what);
    void Equal(const std::string& actual, const std::string& expected, const std::string& what);
    void Equal(uint64_t actual, uint64_t expected, const std::string& what);
    void Contains(const std::string& haystack, const std::string& needle, const std::string& what);
    void NotContains(const std::string& haystack, const std::string& needle, const std::string& what);

    int Report() const;

private:
    void Fail(const std::string& what, const std::string& detail);

    std::vector<std::string> failures_;
    int passed_ = 0;
};

std::string ReadTextFile(const std::string& path);

} // namespace OfflineBehavior
