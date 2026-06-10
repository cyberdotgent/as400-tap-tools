#pragma once

#include "TapeElement.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace tap {

struct ProgressInfo {
    std::uint64_t bytes_read = 0;
    std::uint64_t bytes_total = 0;
};

using ProgressCallback = std::function<void(const ProgressInfo&)>;
using ElementCallback = std::function<void(const TapeElement&, std::size_t index)>;

} // namespace tap
