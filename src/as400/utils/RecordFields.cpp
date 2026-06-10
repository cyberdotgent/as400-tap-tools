#include "RecordFields.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace as400::utils {

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

const RecordField* findField(const std::vector<RecordField>& fields, std::string_view name)
{
    const auto field = std::find_if(fields.begin(), fields.end(), [name](const RecordField& entry) {
        return entry.name == name;
    });
    return field == fields.end() ? nullptr : &*field;
}

const RecordField* findField(const RecordInfo& record, std::string_view name)
{
    return findField(record.fields, name);
}

const RecordField* findChildField(const RecordField& field, std::string_view name)
{
    return findField(field.children, name);
}

std::string fieldValue(const RecordInfo& record, std::string_view name)
{
    const auto* field = findField(record, name);
    return field == nullptr ? std::string{} : field->value;
}

std::string childFieldValue(const RecordField& field, std::string_view name)
{
    const auto* child = findChildField(field, name);
    return child == nullptr ? std::string{} : child->value;
}

std::string decodedDateValue(const RecordInfo& record, std::string_view name, bool expiration_date)
{
    const auto* field = findField(record, name);
    if (field == nullptr) {
        return {};
    }

    if (expiration_date) {
        const auto normalized = normalizedExpirationValue(field->value);
        if (!normalized.empty()) {
            return normalized;
        }
    }

    const auto decoded = childFieldValue(*field, "Date");
    return decoded.empty() ? field->value : decoded;
}

void appendField(std::vector<RecordField>& fields, const char* label, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    fields.push_back(RecordField{label, value});
}

std::string formatDetails(const std::vector<RecordField>& fields)
{
    std::string details;
    for (const auto& field : fields) {
        if (!details.empty()) {
            details += " | ";
        }
        details += field.name;
        details += ": ";
        details += field.value;
    }
    return details;
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

} // namespace as400::utils
