#pragma once

#include "as400/RecordParser.h"

#include <string>
#include <string_view>
#include <vector>

namespace as400::utils {

const RecordField* findField(const std::vector<RecordField>& fields, std::string_view name);
const RecordField* findField(const RecordInfo& record, std::string_view name);
const RecordField* findChildField(const RecordField& field, std::string_view name);

std::string fieldValue(const RecordInfo& record, std::string_view name);
std::string childFieldValue(const RecordField& field, std::string_view name);
std::string decodedDateValue(const RecordInfo& record, std::string_view name, bool expiration_date);

void appendField(std::vector<RecordField>& fields, const char* label, const std::string& value);
std::string formatDetails(const std::vector<RecordField>& fields);
RecordInfo makeInfo(RecordType type, std::string code, std::string name, std::vector<RecordField> fields);

} // namespace as400::utils
