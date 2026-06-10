#include "RecordParser.h"

#include "Cp37.h"
#include "utils/As400Date.h"
#include "utils/FixedWidthText.h"
#include "utils/RecordFields.h"

namespace as400 {
namespace {

RecordInfo parseVolumeLabel(const std::string& text)
{
    std::vector<RecordField> fields;
    utils::appendField(fields, "Record type", "VOL1");
    utils::appendField(fields, "Volume", utils::field(text, 4, 6));
    utils::appendField(fields, "Owner", utils::field(text, 37, 14));
    return utils::makeInfo(RecordType::VolumeLabel, "VOL1", "Volume label", std::move(fields));
}

RecordInfo parseHeader1(const std::string& text)
{
    std::vector<RecordField> fields;
    utils::appendField(fields, "Record type", "HDR1");
    utils::appendField(fields, "File", utils::field(text, 4, 17));
    utils::appendField(fields, "Set", utils::field(text, 21, 6));
    utils::appendField(fields, "Section", utils::field(text, 27, 4));
    utils::appendField(fields, "Sequence", utils::field(text, 31, 4));
    utils::appendField(fields, "Generation", utils::field(text, 35, 4));
    fields.push_back(utils::As400Date::makeDecodedDateField("Created", utils::field(text, 41, 6), false));
    fields.push_back(utils::As400Date::makeDecodedDateField("Expires", utils::field(text, 47, 6), true));
    utils::appendField(fields, "Block count", utils::field(text, 54, 6));
    utils::appendField(fields, "System", utils::field(text, 60, 13));
    return utils::makeInfo(RecordType::Header1, "HDR1", "Data set header 1", std::move(fields));
}

RecordInfo parseHeader2(const std::string& text)
{
    std::vector<RecordField> fields;
    utils::appendField(fields, "Record type", "HDR2");
    utils::appendField(fields, "Format", utils::field(text, 4, 1));
    utils::appendField(fields, "Block length", utils::field(text, 5, 5));
    utils::appendField(fields, "Record length", utils::field(text, 10, 5));
    utils::appendField(fields, "Density", utils::field(text, 15, 1));
    utils::appendField(fields, "Job", utils::field(text, 38, 17));
    return utils::makeInfo(RecordType::Header2, "HDR2", "Data set header 2", std::move(fields));
}

RecordInfo parseSimpleLabel(const std::string& code)
{
    if (code == "EOF1") {
        return utils::makeInfo(RecordType::EndOfFile1, code, "End-of-file label 1", {RecordField{"Record type", code}});
    }
    if (code == "EOF2") {
        return utils::makeInfo(RecordType::EndOfFile2, code, "End-of-file label 2", {RecordField{"Record type", code}});
    }
    if (code == "UHL1") {
        return utils::makeInfo(RecordType::UserHeader1, code, "User header label 1", {RecordField{"Record type", code}});
    }
    if (code == "UHL2") {
        return utils::makeInfo(RecordType::UserHeader2, code, "User header label 2", {RecordField{"Record type", code}});
    }
    return {};
}

} // namespace

RecordInfo RecordParser::parseRecord(const std::vector<std::uint8_t>& data) const
{
    if (data.size() < 4) {
        return {};
    }

    const auto decoded = decodeCp37(data);
    const auto code = decoded.substr(0, 4);
    if (code == "VOL1") {
        return parseVolumeLabel(decoded);
    }
    if (code == "HDR1") {
        return parseHeader1(decoded);
    }
    if (code == "HDR2") {
        return parseHeader2(decoded);
    }

    auto simple = parseSimpleLabel(code);
    if (simple.recognized) {
        return simple;
    }

    RecordInfo info;
    info.code = code;
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
