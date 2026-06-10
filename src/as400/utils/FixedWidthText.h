#pragma once

#include <cstddef>
#include <string>

namespace as400::utils {

std::string trim(std::string value);
std::string field(const std::string& text, std::size_t offset, std::size_t length);
bool isDigits(const std::string& value);

} // namespace as400::utils
