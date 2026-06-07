#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace tap {

struct ProgressInfo {
    std::uint64_t bytes_read = 0;
    std::uint64_t bytes_total = 0;
};

using ProgressCallback = std::function<void(const ProgressInfo&)>;

} // namespace tap
