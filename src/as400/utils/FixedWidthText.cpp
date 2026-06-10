#include "FixedWidthText.h"

#include <algorithm>
#include <cctype>

namespace as400::utils {

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

bool isDigits(const std::string& value)
{
    return std::all_of(value.begin(), value.end(), [](char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    });
}

} // namespace as400::utils
