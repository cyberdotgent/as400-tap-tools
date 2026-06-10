#pragma once

#include "as400/RecordParser.h"

#include <string>

namespace as400::utils {

class As400Date {
public:
    static bool isLeapYear(int year);
    static std::string isoDateString(int year, int month, int day);
    static RecordField makeDecodedDateField(const char* label, const std::string& raw_value, bool expiration_date);
};

} // namespace as400::utils
