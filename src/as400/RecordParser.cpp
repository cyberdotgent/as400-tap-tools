#include "RecordParser.h"

#include "Cp37.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace as400 {
namespace {

std::string trim(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    const auto first = std::find_if(value.begin(), value.end(), [](char ch) {
        return std::isspace(static_cast<unsigned char>(ch)) == 0;
    });
    value.erase(value.begin(), first);
    return value;
}

std::string field(const std::string& text, std::size_t offset, std::size_t length)
{
    if (offset >= text.size()) {
        return {};
    }
    return trim(text.substr(offset, std::min(length, text.size() - offset)));
}

void appendField(std::ostringstream& output, const char* label, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    if (output.tellp() > 0) {
        output << " | ";
    }
    output << label << ": " << value;
}

RecordInfo makeInfo(RecordType type, std::string code, std::string name, std::string details)
{
    RecordInfo info;
    info.type = type;
    info.code = std::move(code);
    info.name = std::move(name);
    info.details = std::move(details);
    info.recognized = true;
    return info;
}

RecordInfo parseVolumeLabel(const std::string& text)
{
    std::ostringstream details;
    appendField(details, "Volume", field(text, 4, 6));
    appendField(details, "Owner", field(text, 37, 14));
    return makeInfo(RecordType::VolumeLabel, "VOL1", "Volume label", details.str());
}

RecordInfo parseHeader1(const std::string& text)
{
    std::ostringstream details;
    appendField(details, "File", field(text, 4, 17));
    appendField(details, "Set", field(text, 21, 6));
    appendField(details, "Section", field(text, 27, 4));
    appendField(details, "Sequence", field(text, 31, 4));
    appendField(details, "Generation", field(text, 35, 4));
    appendField(details, "Created", field(text, 41, 6));
    appendField(details, "Expires", field(text, 47, 6));
    appendField(details, "System", field(text, 60, 13));
    return makeInfo(RecordType::Header1, "HDR1", "Data set header 1", details.str());
}

RecordInfo parseHeader2(const std::string& text)
{
    std::ostringstream details;
    appendField(details, "Format", field(text, 4, 1));
    appendField(details, "Block length", field(text, 5, 5));
    appendField(details, "Record length", field(text, 10, 5));
    appendField(details, "Density", field(text, 15, 1));
    appendField(details, "Job", field(text, 38, 17));
    return makeInfo(RecordType::Header2, "HDR2", "Data set header 2", details.str());
}

RecordInfo parseSimpleLabel(const std::string& code)
{
    if (code == "EOF1") {
        return makeInfo(RecordType::EndOfFile1, code, "End-of-file label 1", {});
    }
    if (code == "EOF2") {
        return makeInfo(RecordType::EndOfFile2, code, "End-of-file label 2", {});
    }
    if (code == "UHL1") {
        return makeInfo(RecordType::UserHeader1, code, "User header label 1", {});
    }
    if (code == "UHL2") {
        return makeInfo(RecordType::UserHeader2, code, "User header label 2", {});
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
