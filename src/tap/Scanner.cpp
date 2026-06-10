#include "Scanner.h"

#include "SimhFormat.h"

#include <fstream>
#include <limits>
#include <utility>

namespace tap {
namespace {

RecordClass toRecordClass(std::uint32_t word)
{
    switch (simh::recordClass(word)) {
    case 0x8:
        return RecordClass::Bad;
    case 0xE:
        return RecordClass::TapeDescription;
    default:
        return RecordClass::Good;
    }
}

bool readPayload(std::istream& input, std::vector<std::uint8_t>& data, std::uint32_t padded_length)
{
    data.resize(padded_length);
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return input.gcount() == static_cast<std::streamsize>(data.size());
}

enum class ScanStep {
    Continue,
    Stop,
    Error,
};

ScanStep readNextElement(std::istream& input, std::uint64_t& offset, TapeImage* image, std::vector<Error>& diagnostics)
{
    const auto word_result = simh::readWord(input, offset);
    if (!word_result) {
        diagnostics.push_back(word_result.error());
        return ScanStep::Error;
    }

    const auto word = word_result.value();
    if (word == simh::TapeMarkWord) {
        if (image) {
            image->insert(image->elementCount(), TapeElement(TapeMark{offset}));
        }
        offset += 4;
        return ScanStep::Continue;
    }
    if (word == simh::EraseGapWord) {
        if (image) {
            image->insert(image->elementCount(), TapeElement(EraseGap{offset}));
        }
        offset += 4;
        return ScanStep::Continue;
    }
    if (word == simh::EndOfMediumWord) {
        if (image) {
            image->insert(image->elementCount(), TapeElement(EndOfMedium{offset}));
        }
        return ScanStep::Stop;
    }

    if (!simh::isStandardDataRecord(word)) {
        diagnostics.push_back(Error{
            ErrorCode::UnsupportedFormat,
            "unsupported SIMH tape object class or marker",
            offset,
        });
        return ScanStep::Error;
    }

    const auto length = simh::recordLength(word);
    if (length > simh::StandardLengthMask) {
        diagnostics.push_back(Error{
            ErrorCode::InvalidRecordLength,
            "SIMH standard record length exceeds 24 bits",
            offset,
        });
        return ScanStep::Error;
    }

    const auto padded_length = (length + 1U) & ~1U;
    std::vector<std::uint8_t> payload;
    if (!readPayload(input, payload, padded_length)) {
        diagnostics.push_back(Error{
            ErrorCode::TrailingPartialRecord,
            "trailing partial SIMH tape record",
            offset,
        });
        return ScanStep::Error;
    }
    payload.resize(length);

    const auto trailer_offset = offset + 4 + padded_length;
    const auto trailer_result = simh::readWord(input, trailer_offset);
    if (!trailer_result) {
        diagnostics.push_back(Error{
            ErrorCode::TrailingPartialRecord,
            "trailing SIMH tape record without matching trailer",
            offset,
        });
        return ScanStep::Error;
    }

    if (trailer_result.value() != word) {
        diagnostics.push_back(Error{
            ErrorCode::MismatchedRecordTrailer,
            "SIMH tape record trailing length does not match leading length",
            trailer_offset,
        });
        return ScanStep::Error;
    }

    if (image) {
        Record record;
        record.data = std::move(payload);
        record.offset = offset;
        record.encoded_length = word;
        record.record_class = toRecordClass(word);
        image->insert(image->elementCount(), TapeElement(std::move(record)));
    }

    offset = trailer_offset + 4;
    return ScanStep::Continue;
}

} // namespace

ScanResult Scanner::scan(
    std::istream& input,
    std::uint64_t total_bytes,
    const ProgressCallback& progress,
    const ElementCallback& on_element) const
{
    ScanResult result;
    std::uint64_t offset = 0;

    while (true) {
        if (input.peek() == std::char_traits<char>::eof()) {
            break;
        }

        const auto element_count_before = result.image.elementCount();
        const auto step = readNextElement(input, offset, &result.image, result.diagnostics);
        if (step == ScanStep::Error) {
            break;
        }

        if (on_element && result.image.elementCount() > element_count_before) {
            on_element(result.image.elements().back(), element_count_before);
        }

        if (progress) {
            progress(ProgressInfo{offset, total_bytes});
        }
        if (step == ScanStep::Stop) {
            break;
        }
    }

    return result;
}

ScanResult Scanner::scan(
    const std::filesystem::path& path,
    std::uint64_t total_bytes,
    const ProgressCallback& progress,
    const ElementCallback& on_element) const
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ScanResult result;
        result.diagnostics.push_back(Error{
            ErrorCode::IoError,
            "failed to open SIMH tape file for reading",
            0,
        });
        return result;
    }

    return scan(input, total_bytes, progress, on_element);
}

} // namespace tap
