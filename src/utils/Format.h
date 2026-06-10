#pragma once

#include <cstdint>
#include <string>

namespace utils {

class Format {
public:
    static std::string humanSize(std::uint64_t value);
};

} // namespace utils
