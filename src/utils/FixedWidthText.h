#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace utils {

std::string trim(std::string value);
bool isDigits(const std::string& value);
std::vector<std::uint8_t> trim(std::vector<std::uint8_t> value);
std::vector<std::uint8_t> field(const std::vector<std::uint8_t>& text, std::size_t offset, std::size_t length);
bool isDigits(const std::vector<std::uint8_t>& value);

} // namespace utils
