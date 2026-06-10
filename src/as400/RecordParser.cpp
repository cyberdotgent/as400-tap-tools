#include "RecordParser.h"

#include "as400/As400Date.h"
#include "utils/FixedWidthText.h"
#include "utils/RecordFields.h"

#include <array>

namespace as400 {
namespace {

constexpr std::array<std::uint8_t, 4> Vol1Code{0xE5, 0xD6, 0xD3, 0xF1};
constexpr std::array<std::uint8_t, 4> Hdr1Code{0xC8, 0xC4, 0xD9, 0xF1};
constexpr std::array<std::uint8_t, 4> Hdr2Code{0xC8, 0xC4, 0xD9, 0xF2};
constexpr std::array<std::uint8_t, 4> Eof1Code{0xC5, 0xD6, 0xC6, 0xF1};
constexpr std::array<std::uint8_t, 4> Eof2Code{0xC5, 0xD6, 0xC6, 0xF2};
constexpr std::array<std::uint8_t, 4> Uhl1Code{0xE4, 0xC8, 0xD3, 0xF1};
constexpr std::array<std::uint8_t, 4> Uhl2Code{0xE4, 0xC8, 0xD3, 0xF2};

bool hasCode(const std::vector<std::uint8_t>& data, const std::array<std::uint8_t, 4>& code)
{
    return data.size() >= code.size() && std::equal(code.begin(), code.end(), data.begin());
}

RecordInfo parseVolumeLabel(const std::vector<std::uint8_t>& data)
{
    std::vector<RecordField> fields;
    ::utils::appendDisplayField(fields, "Record type", "VOL1");
    ::utils::appendField(fields, "Volume", ::utils::field(data, 4, 6));
    ::utils::appendField(fields, "Owner", ::utils::field(data, 37, 14));
    return ::utils::makeInfo(RecordType::VolumeLabel, "VOL1", "Volume label", std::move(fields));
}

RecordInfo parseHeader1(const std::vector<std::uint8_t>& data)
{
    std::vector<RecordField> fields;
    ::utils::appendDisplayField(fields, "Record type", "HDR1");
    ::utils::appendField(fields, "File", ::utils::field(data, 4, 17));
    ::utils::appendField(fields, "Set", ::utils::field(data, 21, 6));
    ::utils::appendField(fields, "Section", ::utils::field(data, 27, 4));
    ::utils::appendField(fields, "Sequence", ::utils::field(data, 31, 4));
    ::utils::appendField(fields, "Generation", ::utils::field(data, 35, 4));
    fields.push_back(as400::utils::As400Date::makeDecodedDateField("Created", ::utils::field(data, 41, 6), false));
    fields.push_back(as400::utils::As400Date::makeDecodedDateField("Expires", ::utils::field(data, 47, 6), true));
    ::utils::appendField(fields, "Block count", ::utils::field(data, 54, 6));
    ::utils::appendField(fields, "System", ::utils::field(data, 60, 13));
    return ::utils::makeInfo(RecordType::Header1, "HDR1", "Data set header 1", std::move(fields));
}

RecordInfo parseHeader2(const std::vector<std::uint8_t>& data)
{
    std::vector<RecordField> fields;
    ::utils::appendDisplayField(fields, "Record type", "HDR2");
    ::utils::appendField(fields, "Format", ::utils::field(data, 4, 1));
    ::utils::appendField(fields, "Block length", ::utils::field(data, 5, 5));
    ::utils::appendField(fields, "Record length", ::utils::field(data, 10, 5));
    ::utils::appendField(fields, "Density", ::utils::field(data, 15, 1));
    ::utils::appendField(fields, "Job", ::utils::field(data, 38, 17));
    return ::utils::makeInfo(RecordType::Header2, "HDR2", "Data set header 2", std::move(fields));
}

RecordInfo parseSimpleLabel(const std::string& code)
{
    if (code == "EOF1") {
        return ::utils::makeInfo(RecordType::EndOfFile1, code, "End-of-file label 1", {::utils::makeDisplayField("Record type", code)});
    }
    if (code == "EOF2") {
        return ::utils::makeInfo(RecordType::EndOfFile2, code, "End-of-file label 2", {::utils::makeDisplayField("Record type", code)});
    }
    if (code == "UHL1") {
        return ::utils::makeInfo(RecordType::UserHeader1, code, "User header label 1", {::utils::makeDisplayField("Record type", code)});
    }
    if (code == "UHL2") {
        return ::utils::makeInfo(RecordType::UserHeader2, code, "User header label 2", {::utils::makeDisplayField("Record type", code)});
    }
    return {};
}

} // namespace

RecordInfo RecordParser::parseRecord(const std::vector<std::uint8_t>& data) const
{
    if (data.size() < 4) {
        return {};
    }

    if (hasCode(data, Vol1Code)) {
        return parseVolumeLabel(data);
    }
    if (hasCode(data, Hdr1Code)) {
        return parseHeader1(data);
    }
    if (hasCode(data, Hdr2Code)) {
        return parseHeader2(data);
    }
    if (hasCode(data, Eof1Code)) {
        return parseSimpleLabel("EOF1");
    }
    if (hasCode(data, Eof2Code)) {
        return parseSimpleLabel("EOF2");
    }
    if (hasCode(data, Uhl1Code)) {
        return parseSimpleLabel("UHL1");
    }
    if (hasCode(data, Uhl2Code)) {
        return parseSimpleLabel("UHL2");
    }

    RecordInfo info;
    info.code = "????";
    info.name = "Unknown AS/400 record";
    return info;
}

bool RecordParser::isAs400Tape(const tap::TapeImage& image) const
{
    std::size_t recognized_count = 0;
    bool first_record_is_volume_label = false;
    bool has_header = false;

    for (const auto& element : image.elements()) {
        if (!element.isRecord()) {
            continue;
        }

        const auto info = parseRecord(element.record().data);
        if (!info.recognized) {
            continue;
        }

        ++recognized_count;
        if (recognized_count == 1 && info.type == RecordType::VolumeLabel) {
            first_record_is_volume_label = true;
        }
        if (info.type == RecordType::Header1 || info.type == RecordType::Header2) {
            has_header = true;
        }
        if (first_record_is_volume_label && (has_header || recognized_count >= 1)) {
            return true;
        }
    }

    return false;
}

} // namespace as400
