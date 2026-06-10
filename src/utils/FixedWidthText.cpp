#include "FixedWidthText.h"

#include <algorithm>
#include <cctype>

namespace utils {
namespace {

bool isTrimmedByte(std::uint8_t value)
{
    return value == 0x40 || value == 0x00;
}

}

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

bool isDigits(const std::string& value)
{
    return std::all_of(value.begin(), value.end(), [](char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    });
}

std::vector<std::uint8_t> trim(std::vector<std::uint8_t> value)
{
    while (!value.empty() && isTrimmedByte(value.back())) {
        value.pop_back();
    }
    const auto first = std::find_if(value.begin(), value.end(), [](std::uint8_t ch) {
        return !isTrimmedByte(ch);
    });
    value.erase(value.begin(), first);
    return value;
}

std::vector<std::uint8_t> field(const std::vector<std::uint8_t>& text, std::size_t offset, std::size_t length)
{
    if (offset >= text.size()) {
        return {};
    }

    return trim(std::vector<std::uint8_t>(
        text.begin() + static_cast<std::vector<std::uint8_t>::difference_type>(offset),
        text.begin() + static_cast<std::vector<std::uint8_t>::difference_type>(offset + std::min(length, text.size() - offset))));
}

bool isDigits(const std::vector<std::uint8_t>& value)
{
    return std::all_of(value.begin(), value.end(), [](std::uint8_t ch) {
        return ch >= 0xF0 && ch <= 0xF9;
    });
}

} // namespace utils
