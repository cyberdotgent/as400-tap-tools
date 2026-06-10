#include "RecordFields.h"

#include "utils/ebcdic/Ebcdic.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace utils {

namespace {

std::string normalizedExpirationValue(std::string value)
{
    if (value == "Does not expire") {
        return value;
    }

    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value == "never" ? std::string{"Does not expire"} : std::string{};
}

} // namespace

const as400::RecordField* findField(const std::vector<as400::RecordField>& fields, std::string_view name)
{
    const auto field = std::find_if(fields.begin(), fields.end(), [name](const as400::RecordField& entry) {
        return entry.name == name;
    });
    return field == fields.end() ? nullptr : &*field;
}

const as400::RecordField* findField(const as400::RecordInfo& record, std::string_view name)
{
    return findField(record.fields, name);
}

const as400::RecordField* findChildField(const as400::RecordField& field, std::string_view name)
{
    return findField(field.children, name);
}

std::string renderFieldValue(const as400::RecordField& field, utils::ebcdic::CCSID ccsid)
{
    if (field.has_display_value) {
        return field.display_value;
    }

    return utils::ebcdic::decode(field.raw_value, ccsid);
}

std::string fieldValue(const as400::RecordInfo& record, std::string_view name, utils::ebcdic::CCSID ccsid)
{
    const auto* field = findField(record, name);
    return field == nullptr ? std::string{} : renderFieldValue(*field, ccsid);
}

std::string childFieldValue(const as400::RecordField& field, std::string_view name, utils::ebcdic::CCSID ccsid)
{
    const auto* child = findChildField(field, name);
    return child == nullptr ? std::string{} : renderFieldValue(*child, ccsid);
}

std::string decodedDateValue(const as400::RecordInfo& record, std::string_view name, utils::ebcdic::CCSID ccsid, bool expiration_date)
{
    const auto* field = findField(record, name);
    if (field == nullptr) {
        return {};
    }

    const auto field_value = renderFieldValue(*field, ccsid);
    if (expiration_date) {
        const auto normalized = normalizedExpirationValue(field_value);
        if (!normalized.empty()) {
            return normalized;
        }
    }

    const auto decoded = childFieldValue(*field, "Date", ccsid);
    return decoded.empty() ? field_value : decoded;
}

as400::RecordField makeRawField(const char* label, std::vector<std::uint8_t> raw_value)
{
    as400::RecordField field;
    field.name = label;
    field.raw_value = std::move(raw_value);
    return field;
}

as400::RecordField makeDisplayField(const char* label, std::string value)
{
    as400::RecordField field;
    field.name = label;
    field.display_value = std::move(value);
    field.has_display_value = true;
    return field;
}

void appendField(std::vector<as400::RecordField>& fields, const char* label, std::vector<std::uint8_t> raw_value)
{
    if (raw_value.empty()) {
        return;
    }
    fields.push_back(makeRawField(label, std::move(raw_value)));
}

void appendDisplayField(std::vector<as400::RecordField>& fields, const char* label, std::string value)
{
    if (value.empty()) {
        return;
    }
    fields.push_back(makeDisplayField(label, std::move(value)));
}

as400::RecordInfo makeInfo(
    as400::RecordType type,
    std::string code,
    std::string name,
    std::vector<as400::RecordField> fields)
{
    as400::RecordInfo info;
    info.type = type;
    info.code = std::move(code);
    info.name = std::move(name);
    info.fields = std::move(fields);
    info.recognized = true;
    return info;
}

} // namespace utils
