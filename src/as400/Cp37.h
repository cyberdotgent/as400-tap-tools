#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace as400 {

char decodeCp37Byte(std::uint8_t value);
std::string decodeCp37(const std::vector<std::uint8_t>& data);
std::string decodeCp37(const std::uint8_t* data, std::size_t size);
std::vector<std::uint8_t> encodeCp37(std::string_view text);

} // namespace as400
