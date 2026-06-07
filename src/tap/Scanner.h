#pragma once

#include "Error.h"
#include "Progress.h"
#include "Result.h"
#include "TapeImage.h"

#include <filesystem>
#include <iosfwd>
#include <vector>

namespace tap {

struct ScanResult {
    TapeImage image;
    std::vector<Error> diagnostics;
};

class Scanner {
public:
    ScanResult scan(std::istream& input, std::uint64_t total_bytes = 0, const ProgressCallback& progress = {}) const;
    ScanResult scan(const std::filesystem::path& path, std::uint64_t total_bytes = 0, const ProgressCallback& progress = {}) const;
};

} // namespace tap
