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

Result<TapeImage> Reader::read(std::istream& input, const ProgressCallback& progress) const
{
    Scanner scanner;
    auto scan_result = scanner.scan(input, 0, progress);
    if (!scan_result.diagnostics.empty()) {
        const auto& diagnostic = scan_result.diagnostics.front();
        if (!options_.allow_zuluscsi_trailing_partial_record || !isZuluScsiTrailingPartialRecord(diagnostic)) {
            return Result<TapeImage>::fail(diagnostic);
        }
    }

    return Result<TapeImage>::ok(std::move(scan_result.image));
}

Result<TapeImage> Reader::read(const std::filesystem::path& path, const ProgressCallback& progress) const
{
    Scanner scanner;
    const auto count_result = scanner.count(path, progress);
    if (!count_result) {
        return Result<TapeImage>::fail(count_result.error());
    }

    auto scan_result = scanner.scan(path, count_result.value(), progress);
    if (!scan_result.diagnostics.empty()) {
        const auto& diagnostic = scan_result.diagnostics.front();
        if (!options_.allow_zuluscsi_trailing_partial_record || !isZuluScsiTrailingPartialRecord(diagnostic)) {
            return Result<TapeImage>::fail(diagnostic);
        }
    }

    return Result<TapeImage>::ok(std::move(scan_result.image));
}

const ReaderOptions& Reader::options() const
{
    return options_;
}

} // namespace tap
