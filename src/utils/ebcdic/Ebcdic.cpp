#include "Ebcdic.h"

#include <array>

namespace utils::ebcdic {
namespace {

std::array<std::uint8_t, 128> buildAsciiToEbcdic(const std::array<unsigned char, 256>& to_latin1)
{
    std::array<std::uint8_t, 128> result{};
    result.fill(0x6F);
    for (std::size_t index = 0; index < to_latin1.size(); ++index) {
        const auto decoded = to_latin1[index];
        if (decoded < result.size()) {
            result[decoded] = static_cast<std::uint8_t>(index);
        }
    }
    return result;
}

const std::array<std::uint8_t, 128>& asciiToEbcdicTable(CCSID ccsid)
{
    static const auto ascii_to_ccsid37 = buildAsciiToEbcdic(toLatin1Table(CCSID::Ccsid37));

    switch (ccsid) {
    case CCSID::Ccsid37:
        return ascii_to_ccsid37;
    }

    return ascii_to_ccsid37;
}

} // namespace

char decodeByte(std::uint8_t value, CCSID ccsid)
{
    const auto decoded = toLatin1Table(ccsid)[value];
    return decoded >= 0x20 && decoded < 0x7F ? static_cast<char>(decoded) : '.';
}

std::string decode(const std::vector<std::uint8_t>& data, CCSID ccsid)
{
    return decode(data.data(), data.size(), ccsid);
}

std::string decode(const std::uint8_t* data, std::size_t size, CCSID ccsid)
{
    std::string result;
    result.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        result.push_back(decodeByte(data[index], ccsid));
    }
    return result;
}

std::vector<std::uint8_t> encode(std::string_view text, CCSID ccsid)
{
    const auto& ascii_to_ebcdic = asciiToEbcdicTable(ccsid);
    std::vector<std::uint8_t> result;
    result.reserve(text.size());
    for (const auto ch : text) {
        const auto value = static_cast<unsigned char>(ch);
        result.push_back(value < ascii_to_ebcdic.size() ? ascii_to_ebcdic[value] : 0x6F);
    }
    return result;
}

} // namespace utils::ebcdic
