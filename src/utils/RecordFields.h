#pragma once

#include "as400/RecordParser.h"
#include "utils/ebcdic/Ccsids.h"

#include <string>
#include <string_view>
#include <vector>

namespace utils {

const as400::RecordField* findField(const std::vector<as400::RecordField>& fields, std::string_view name);
const as400::RecordField* findField(const as400::RecordInfo& record, std::string_view name);
const as400::RecordField* findChildField(const as400::RecordField& field, std::string_view name);

std::string renderFieldValue(const as400::RecordField& field, utils::ebcdic::CCSID ccsid);
std::string fieldValue(const as400::RecordInfo& record, std::string_view name, utils::ebcdic::CCSID ccsid);
std::string childFieldValue(const as400::RecordField& field, std::string_view name, utils::ebcdic::CCSID ccsid);
std::string decodedDateValue(const as400::RecordInfo& record, std::string_view name, utils::ebcdic::CCSID ccsid, bool expiration_date);

as400::RecordField makeRawField(const char* label, std::vector<std::uint8_t> raw_value);
as400::RecordField makeDisplayField(const char* label, std::string value);
void appendField(std::vector<as400::RecordField>& fields, const char* label, std::vector<std::uint8_t> raw_value);
void appendDisplayField(std::vector<as400::RecordField>& fields, const char* label, std::string value);
as400::RecordInfo makeInfo(
    as400::RecordType type,
    std::string code,
    std::string name,
    std::vector<as400::RecordField> fields);

} // namespace utils
