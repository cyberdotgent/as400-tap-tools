#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace tap {

struct ProgressInfo {
    std::size_t current = 0;
    std::size_t total = 0;
    std::uint64_t offset = 0;
    bool counting = false;
};

using ProgressCallback = std::function<void(const ProgressInfo&)>;

} // namespace tap
