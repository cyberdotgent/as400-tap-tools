#pragma once

#include "as400/RecordParser.h"

#include <string>
#include <string_view>
#include <vector>

namespace utils {

const as400::RecordField* findField(const std::vector<as400::RecordField>& fields, std::string_view name);
const as400::RecordField* findField(const as400::RecordInfo& record, std::string_view name);
const as400::RecordField* findChildField(const as400::RecordField& field, std::string_view name);

std::string fieldValue(const as400::RecordInfo& record, std::string_view name);
std::string childFieldValue(const as400::RecordField& field, std::string_view name);
std::string decodedDateValue(const as400::RecordInfo& record, std::string_view name, bool expiration_date);

void appendField(std::vector<as400::RecordField>& fields, const char* label, const std::string& value);
std::string formatDetails(const std::vector<as400::RecordField>& fields);
as400::RecordInfo makeInfo(
    as400::RecordType type,
    std::string code,
    std::string name,
    std::vector<as400::RecordField> fields);

} // namespace utils
