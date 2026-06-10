#include "Format.h"

#include <iomanip>
#include <sstream>

namespace as400::utils {

std::string Format::humanSize(std::uint64_t value)
{
    static constexpr const char* suffixes[] = {"B", "K", "M", "G"};
    static constexpr std::size_t suffix_count = sizeof(suffixes) / sizeof(suffixes[0]);
    double scaled = static_cast<double>(value);
    std::size_t suffix_index = 0;
    while (scaled >= 1024.0 && suffix_index + 1 < suffix_count) {
        scaled /= 1024.0;
        ++suffix_index;
    }

    std::ostringstream output;
    if (suffix_index == 0) {
        output << value << ' ' << suffixes[suffix_index];
    } else {
        output << std::fixed << std::setprecision(1) << scaled << ' ' << suffixes[suffix_index];
    }
    return output.str();
}

} // namespace as400::utils
