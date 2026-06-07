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
    Result<std::size_t> count(std::istream& input, const ProgressCallback& progress = {}) const;
    Result<std::size_t> count(const std::filesystem::path& path, const ProgressCallback& progress = {}) const;

    ScanResult scan(std::istream& input, std::size_t total_objects = 0, const ProgressCallback& progress = {}) const;
    ScanResult scan(const std::filesystem::path& path, std::size_t total_objects = 0, const ProgressCallback& progress = {}) const;
};

} // namespace tap
