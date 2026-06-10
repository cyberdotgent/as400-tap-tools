#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace utils::ebcdic {

enum class CCSID {
    Ccsid37
};

struct CCSIDMetadata {
    CCSID ccsid;
    int number;
    const char* description;
};

const std::vector<CCSIDMetadata>& availableCcsids();
const CCSIDMetadata& ccsidMetadata(CCSID ccsid);
std::string ccsidMenuLabel(CCSID ccsid);
const std::array<unsigned char, 256>& toLatin1Table(CCSID ccsid = CCSID::Ccsid37);

} // namespace utils::ebcdic
