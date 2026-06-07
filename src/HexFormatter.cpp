#include "HexFormatter.h"

#include "as400/Cp37.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace {

constexpr std::size_t BytesPerLine = 16;
constexpr std::size_t OffsetChars = 8;
constexpr std::size_t HexColumnChars = BytesPerLine * 3 + 1;

char printableAscii(std::uint8_t value)
{
    return std::isprint(static_cast<unsigned char>(value)) != 0 ? static_cast<char>(value) : '.';
}

char printable(std::uint8_t value, TextEncoding encoding)
{
    return encoding == TextEncoding::EbcdicCp37 ? as400::decodeCp37Byte(value) : printableAscii(value);
}

std::size_t textColumnOffset(std::size_t line_start, std::size_t byte_offset)
{
    const auto byte_in_line = byte_offset % BytesPerLine;
    return line_start + OffsetChars + 2 + HexColumnChars + 1 + byte_in_line;
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

    return as400::encodeCp37(text);
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
