#include "HexFormatter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace {

constexpr std::size_t BytesPerLine = 16;
constexpr std::size_t OffsetChars = 8;
constexpr std::size_t HexColumnChars = BytesPerLine * 3 + 1;

constexpr std::array<unsigned char, 256> Cp37ToUnicodeLatin1{
    0x00, 0x01, 0x02, 0x03, 0x9C, 0x09, 0x86, 0x7F, 0x97, 0x8D, 0x8E, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x9D, 0x85, 0x08, 0x87, 0x18, 0x19, 0x92, 0x8F, 0x1C, 0x1D, 0x1E, 0x1F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x0A, 0x17, 0x1B, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x05, 0x06, 0x07,
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, 0x98, 0x99, 0x9A, 0x9B, 0x14, 0x15, 0x9E, 0x1A,
    0x20, 0xA0, 0xE2, 0xE4, 0xE0, 0xE1, 0xE3, 0xE5, 0xE7, 0xF1, 0xA2, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
    0x26, 0xE9, 0xEA, 0xEB, 0xE8, 0xED, 0xEE, 0xEF, 0xEC, 0xDF, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
    0x2D, 0x2F, 0xC2, 0xC4, 0xC0, 0xC1, 0xC3, 0xC5, 0xC7, 0xD1, 0xA6, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
    0xF8, 0xC9, 0xCA, 0xCB, 0xC8, 0xCD, 0xCE, 0xCF, 0xCC, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
    0xD8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0xAB, 0xBB, 0xF0, 0xFD, 0xFE, 0xB1,
    0xB0, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0xAA, 0xBA, 0xE6, 0xB8, 0xC6, 0xA4,
    0xB5, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xA1, 0xBF, 0xD0, 0x5B, 0xDE, 0xAE,
    0xAC, 0xA3, 0xA5, 0xB7, 0xA9, 0xA7, 0xB6, 0xBC, 0xBD, 0xBE, 0xDD, 0xA8, 0xAF, 0x5D, 0xB4, 0xD7,
    0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0xAD, 0xF4, 0xF6, 0xF2, 0xF3, 0xF5,
    0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0xB9, 0xFB, 0xFC, 0xF9, 0xFA, 0xFF,
    0x5C, 0xF7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0xB2, 0xD4, 0xD6, 0xD2, 0xD3, 0xD5,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xB3, 0xDB, 0xDC, 0xD9, 0xDA, 0x9F,
};

char printableAscii(std::uint8_t value)
{
    return std::isprint(static_cast<unsigned char>(value)) != 0 ? static_cast<char>(value) : '.';
}

char printableCp37(std::uint8_t value)
{
    const auto decoded = Cp37ToUnicodeLatin1[value];
    return std::isprint(decoded) != 0 && decoded < 0x80 ? static_cast<char>(decoded) : '.';
}

char printable(std::uint8_t value, TextEncoding encoding)
{
    return encoding == TextEncoding::EbcdicCp37 ? printableCp37(value) : printableAscii(value);
}

std::size_t textColumnOffset(std::size_t line_start, std::size_t byte_offset)
{
    const auto byte_in_line = byte_offset % BytesPerLine;
    return line_start + OffsetChars + 2 + HexColumnChars + 1 + byte_in_line;
}

std::array<std::uint8_t, 128> buildAsciiToCp37()
{
    std::array<std::uint8_t, 128> result{};
    result.fill(0x6F);
    for (std::size_t index = 0; index < Cp37ToUnicodeLatin1.size(); ++index) {
        const auto decoded = Cp37ToUnicodeLatin1[index];
        if (decoded < result.size()) {
            result[decoded] = static_cast<std::uint8_t>(index);
        }
    }
    return result;
}

} // namespace

std::string formatHexView(const std::vector<std::uint8_t>& data, TextEncoding encoding)
{
    if (data.empty()) {
        return {};
    }

    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0');

    for (std::size_t offset = 0; offset < data.size(); offset += BytesPerLine) {
        output << std::setw(OffsetChars) << offset << "  ";

        const auto line_count = std::min(BytesPerLine, data.size() - offset);
        for (std::size_t index = 0; index < BytesPerLine; ++index) {
            if (index == 8) {
                output << ' ';
            }

            if (index < line_count) {
                output << std::setw(2) << static_cast<unsigned int>(data[offset + index]) << ' ';
            } else {
                output << "   ";
            }
        }

        output << ' ';
        for (std::size_t index = 0; index < line_count; ++index) {
            output << printable(data[offset + index], encoding);
        }
        output << '\n';
    }

    return output.str();
}

std::vector<std::uint8_t> encodeSearchText(std::string_view text, TextEncoding encoding)
{
    std::vector<std::uint8_t> result;
    result.reserve(text.size());

    if (encoding == TextEncoding::Ascii) {
        for (const auto ch : text) {
            result.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
        }
        return result;
    }

    static const auto ascii_to_cp37 = buildAsciiToCp37();
    for (const auto ch : text) {
        const auto value = static_cast<unsigned char>(ch);
        result.push_back(value < ascii_to_cp37.size() ? ascii_to_cp37[value] : 0x6F);
    }
    return result;
}

HexSearchResult findBytesInHexView(
    const std::vector<std::uint8_t>& data,
    const std::vector<std::uint8_t>& needle,
    TextEncoding encoding,
    std::size_t start_byte_offset)
{
    if (data.empty() || needle.empty() || needle.size() > data.size()) {
        return {};
    }

    if (start_byte_offset >= data.size()) {
        start_byte_offset = 0;
    }

    const auto find_from = [&](std::size_t start) {
        return std::search(data.begin() + static_cast<std::vector<std::uint8_t>::difference_type>(start),
            data.end(),
            needle.begin(),
            needle.end());
    };

    auto found = find_from(start_byte_offset);
    if (found == data.end() && start_byte_offset != 0) {
        found = find_from(0);
    }
    if (found == data.end()) {
        return {};
    }

    const auto byte_offset = static_cast<std::size_t>(std::distance(data.begin(), found));
    const auto formatted_prefix = formatHexView(
        std::vector<std::uint8_t>(data.begin(), data.begin() + static_cast<std::vector<std::uint8_t>::difference_type>(byte_offset)),
        encoding);
    const auto line_start = formatted_prefix.rfind('\n') == std::string::npos ? 0 : formatted_prefix.rfind('\n') + 1;

    HexSearchResult result;
    result.found = true;
    result.byte_offset = byte_offset;
    result.text_offset = textColumnOffset(line_start, byte_offset);
    result.text_length = std::min(needle.size(), BytesPerLine - (byte_offset % BytesPerLine));
    return result;
}
