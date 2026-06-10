#pragma once

#include "Result.h"
#include "Progress.h"
#include "TapeImage.h"

#include <filesystem>
#include <iosfwd>

namespace tap {

struct ReaderOptions {
    bool allow_zuluscsi_trailing_partial_record = true;
};

class Reader {
public:
    Reader() = default;
    explicit Reader(ReaderOptions options);

    Result<TapeImage> read(
        std::istream& input,
        const ProgressCallback& progress = {},
        const ElementCallback& on_element = {}) const;
    Result<TapeImage> read(
        const std::filesystem::path& path,
        const ProgressCallback& progress = {},
        const ElementCallback& on_element = {}) const;

    const ReaderOptions& options() const;

private:
    ReaderOptions options_;
};

} // namespace tap
