#pragma once

#include "utils/ebcdic/Ccsids.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace utils::ebcdic {

char decodeByte(std::uint8_t value, CCSID ccsid = CCSID::Ccsid37);
std::string decode(const std::vector<std::uint8_t>& data, CCSID ccsid = CCSID::Ccsid37);
std::string decode(const std::uint8_t* data, std::size_t size, CCSID ccsid = CCSID::Ccsid37);
std::vector<std::uint8_t> encode(std::string_view text, CCSID ccsid = CCSID::Ccsid37);

} // namespace utils::ebcdic
