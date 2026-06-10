#include "As400Date.h"

#include "utils/FixedWidthText.h"
#include "utils/RecordFields.h"

#include <iomanip>
#include <sstream>

namespace as400::utils {
namespace {

int digitFromEbcdic(std::uint8_t value)
{
    return static_cast<int>(value - 0xF0);
}

std::string asciiDigits(const std::vector<std::uint8_t>& raw_value)
{
    std::string result;
    result.reserve(raw_value.size());
    for (const auto byte : raw_value) {
        if (byte < 0xF0 || byte > 0xF9) {
            return {};
        }
        result.push_back(static_cast<char>('0' + digitFromEbcdic(byte)));
    }
    return result;
}

}

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

RecordField As400Date::makeDecodedDateField(const char* label, const std::vector<std::uint8_t>& raw_value, bool expiration_date)
{
    auto result = ::utils::makeRawField(label, raw_value);
    if (raw_value.empty()) {
        return result;
    }

    const auto ascii_value = asciiDigits(raw_value);
    if (expiration_date && ascii_value == "99999") {
        result.display_value = "Does not expire";
        result.has_display_value = true;
        result.children.push_back(::utils::makeRawField("Raw", raw_value));
        return result;
    }

    if (raw_value.size() != 6 || !::utils::isDigits(raw_value) || ascii_value.size() != 6) {
        return result;
    }

    const auto century = digitFromEbcdic(raw_value[0]);
    const auto year_in_century = std::stoi(ascii_value.substr(1, 2));
    const auto day_of_year = std::stoi(ascii_value.substr(3, 3));
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

    result.children.push_back(::utils::makeRawField("Raw", raw_value));
    result.children.push_back(::utils::makeDisplayField("Year", std::to_string(year)));
    result.children.push_back(::utils::makeDisplayField("Day of year", std::to_string(day_of_year)));
    result.children.push_back(::utils::makeDisplayField("Month", std::to_string(month)));
    result.children.push_back(::utils::makeDisplayField("Day", std::to_string(remaining)));
    result.children.push_back(::utils::makeDisplayField("Date", isoDateString(year, month, remaining)));
    return result;
}

} // namespace as400::utils
