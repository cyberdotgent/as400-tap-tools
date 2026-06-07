#include "Reader.h"

#include "Scanner.h"

#include <fstream>
#include <utility>

namespace tap {
namespace {

bool isZuluScsiTrailingPartialRecord(const Error& diagnostic)
{
    return diagnostic.code == ErrorCode::TrailingPartialRecord;
}

} // namespace

Reader::Reader(ReaderOptions options)
    : options_(options)
{
}

Result<TapeImage> Reader::read(std::istream& input) const
{
    Scanner scanner;
    auto scan_result = scanner.scan(input);
    if (!scan_result.diagnostics.empty()) {
        const auto& diagnostic = scan_result.diagnostics.front();
        if (!options_.allow_zuluscsi_trailing_partial_record || !isZuluScsiTrailingPartialRecord(diagnostic)) {
            return Result<TapeImage>::fail(diagnostic);
        }
    }

    return Result<TapeImage>::ok(std::move(scan_result.image));
}

Result<TapeImage> Reader::read(const std::filesystem::path& path) const
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return Result<TapeImage>::fail(Error{
            ErrorCode::IoError,
            "failed to open SIMH tape file for reading",
            0,
        });
    }

    return read(input);
}

const ReaderOptions& Reader::options() const
{
    return options_;
}

} // namespace tap
