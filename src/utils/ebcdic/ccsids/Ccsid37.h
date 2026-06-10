#pragma once

#include <array>

namespace utils::ebcdic::ccsids {

constexpr int Ccsid37Number = 37;
constexpr const char* Ccsid37Description = "EBCDIC US/Canada";

const std::array<unsigned char, 256>& ccsid37ToLatin1Table();

} // namespace utils::ebcdic::ccsids
