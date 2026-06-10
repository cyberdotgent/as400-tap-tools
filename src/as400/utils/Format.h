#pragma once

#include <cstdint>
#include <string>

namespace as400::utils {

class Format {
public:
    static std::string humanSize(std::uint64_t value);
};

} // namespace as400::utils
