#include "Reader.h"

#include "Scanner.h"

#include <filesystem>
#include <system_error>
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
    std::error_code error_code;
    const auto total_bytes = std::filesystem::file_size(path, error_code);
    const auto scan_result = scanner.scan(path, error_code ? 0 : total_bytes, progress);
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
