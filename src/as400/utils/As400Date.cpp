#include "As400Date.h"

#include "FixedWidthText.h"

#include <iomanip>
#include <sstream>

namespace as400::utils {

bool As400Date::isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

std::string As400Date::isoDateString(int year, int month, int day)
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

RecordField As400Date::makeDecodedDateField(const char* label, const std::string& raw_value, bool expiration_date)
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
    result.children.push_back(RecordField{"Year", std::to_string(year), {}});
    result.children.push_back(RecordField{"Day of year", std::to_string(day_of_year), {}});
    result.children.push_back(RecordField{"Month", std::to_string(month), {}});
    result.children.push_back(RecordField{"Day", std::to_string(remaining), {}});
    result.children.push_back(RecordField{"Date", isoDateString(year, month, remaining), {}});
    return result;
}

} // namespace as400::utils
