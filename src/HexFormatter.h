#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

enum class TextEncoding {
    Ascii,
    EbcdicCp37
};

struct HexSearchResult {
    bool found = false;
    std::size_t byte_offset = 0;
    std::size_t text_offset = 0;
    std::size_t text_length = 0;
};

std::string formatHexView(const std::vector<std::uint8_t>& data, TextEncoding encoding);
std::vector<std::uint8_t> encodeSearchText(std::string_view text, TextEncoding encoding);
HexSearchResult findBytesInHexView(
    const std::vector<std::uint8_t>& data,
    const std::vector<std::uint8_t>& needle,
    TextEncoding encoding,
    std::size_t start_byte_offset = 0);
