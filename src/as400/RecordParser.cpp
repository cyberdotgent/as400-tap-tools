#include "RecordParser.h"

#include "Cp37.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
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

void appendField(std::vector<RecordField>& fields, const char* label, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    fields.push_back(RecordField{label, value});
}

bool isDigits(const std::string& value)
{
    return std::all_of(value.begin(), value.end(), [](char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    });
}

bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

std::string decimalString(int value)
{
    return std::to_string(value);
}

std::string isoDateString(int year, int month, int day)
{
    std::ostringstream output;
    output << std::setfill('0')
           << std::setw(4) << year
           << '-'
           << std::setw(2) << month
           << '-'
           << std::setw(2) << day;
    return output.str();
}

RecordField makeDecodedDateField(const char* label, const std::string& raw_value, bool expiration_date)
{
    RecordField result{label, raw_value, {}};
    if (raw_value.empty()) {
        return result;
    }

    if (expiration_date && raw_value == "99999") {
        result.value = "Does not expire";
        result.children.push_back(RecordField{"Raw", raw_value, {}});
        return result;
    }

    if (raw_value.size() != 6 || !isDigits(raw_value)) {
        return result;
    }

    const auto century = raw_value[0] - '0';
    const auto year_in_century = std::stoi(raw_value.substr(1, 2));
    const auto day_of_year = std::stoi(raw_value.substr(3, 3));
    const auto year = 2000 + (century * 100) + year_in_century;
    const auto max_day = isLeapYear(year) ? 366 : 365;
    if (day_of_year < 1 || day_of_year > max_day) {
        return result;
    }

    static constexpr int month_lengths_common[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int remaining = day_of_year;
    int month = 1;
    for (const auto month_length_common : month_lengths_common) {
        auto month_length = month_length_common;
        if (month == 2 && isLeapYear(year)) {
            month_length = 29;
        }
        if (remaining <= month_length) {
            break;
        }
        remaining -= month_length;
        ++month;
    }

    result.children.push_back(RecordField{"Raw", raw_value, {}});
    result.children.push_back(RecordField{"Year", decimalString(year), {}});
    result.children.push_back(RecordField{"Day of year", decimalString(day_of_year), {}});
    result.children.push_back(RecordField{"Month", decimalString(month), {}});
    result.children.push_back(RecordField{"Day", decimalString(remaining), {}});
    result.children.push_back(RecordField{"Date", isoDateString(year, month, remaining), {}});
    return result;
}

std::string formatDetails(const std::vector<RecordField>& fields)
{
    std::ostringstream output;
    for (const auto& field : fields) {
        if (output.tellp() > 0) {
            output << " | ";
        }
        output << field.name << ": " << field.value;
    }
    return output.str();
}

RecordInfo makeInfo(RecordType type, std::string code, std::string name, std::vector<RecordField> fields)
{
    RecordInfo info;
    info.type = type;
    info.code = std::move(code);
    info.name = std::move(name);
    info.fields = std::move(fields);
    info.details = formatDetails(info.fields);
    info.recognized = true;
    return info;
}

RecordInfo parseVolumeLabel(const std::string& text)
{
    std::vector<RecordField> fields;
    appendField(fields, "Record type", "VOL1");
    appendField(fields, "Volume", field(text, 4, 6));
    appendField(fields, "Owner", field(text, 37, 14));
    return makeInfo(RecordType::VolumeLabel, "VOL1", "Volume label", std::move(fields));
}

RecordInfo parseHeader1(const std::string& text)
{
    std::vector<RecordField> fields;
    appendField(fields, "Record type", "HDR1");
    appendField(fields, "File", field(text, 4, 17));
    appendField(fields, "Set", field(text, 21, 6));
    appendField(fields, "Section", field(text, 27, 4));
    appendField(fields, "Sequence", field(text, 31, 4));
    appendField(fields, "Generation", field(text, 35, 4));
    fields.push_back(makeDecodedDateField("Created", field(text, 41, 6), false));
    fields.push_back(makeDecodedDateField("Expires", field(text, 47, 6), true));
    appendField(fields, "Block count", field(text, 54, 6));
    appendField(fields, "System", field(text, 60, 13));
    return makeInfo(RecordType::Header1, "HDR1", "Data set header 1", std::move(fields));
}

RecordInfo parseHeader2(const std::string& text)
{
    std::vector<RecordField> fields;
    appendField(fields, "Record type", "HDR2");
    appendField(fields, "Format", field(text, 4, 1));
    appendField(fields, "Block length", field(text, 5, 5));
    appendField(fields, "Record length", field(text, 10, 5));
    appendField(fields, "Density", field(text, 15, 1));
    appendField(fields, "Job", field(text, 38, 17));
    return makeInfo(RecordType::Header2, "HDR2", "Data set header 2", std::move(fields));
}

RecordInfo parseSimpleLabel(const std::string& code)
{
    if (code == "EOF1") {
        return makeInfo(RecordType::EndOfFile1, code, "End-of-file label 1", {RecordField{"Record type", code}});
    }
    if (code == "EOF2") {
        return makeInfo(RecordType::EndOfFile2, code, "End-of-file label 2", {RecordField{"Record type", code}});
    }
    if (code == "UHL1") {
        return makeInfo(RecordType::UserHeader1, code, "User header label 1", {RecordField{"Record type", code}});
    }
    if (code == "UHL2") {
        return makeInfo(RecordType::UserHeader2, code, "User header label 2", {RecordField{"Record type", code}});
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
