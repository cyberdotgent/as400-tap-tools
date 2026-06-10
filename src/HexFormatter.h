#pragma once

#include "utils/ebcdic/Ccsids.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

enum class TextEncoding {
    Ascii,
    Ebcdic
};

struct HexSearchResult {
    bool found = false;
    std::size_t byte_offset = 0;
    std::size_t text_offset = 0;
    std::size_t text_length = 0;
};

std::string formatHexView(
    const std::vector<std::uint8_t>& data,
    TextEncoding encoding,
    utils::ebcdic::CCSID ebcdic_ccsid = utils::ebcdic::CCSID::Ccsid37);
std::vector<std::uint8_t> encodeSearchText(
    std::string_view text,
    TextEncoding encoding,
    utils::ebcdic::CCSID ebcdic_ccsid = utils::ebcdic::CCSID::Ccsid37);
HexSearchResult findBytesInHexView(
    const std::vector<std::uint8_t>& data,
    const std::vector<std::uint8_t>& needle,
    TextEncoding encoding,
    utils::ebcdic::CCSID ebcdic_ccsid = utils::ebcdic::CCSID::Ccsid37,
    std::size_t start_byte_offset = 0);
