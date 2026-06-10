#include "Ccsids.h"

#include "utils/ebcdic/ccsids/Ccsid37.h"

#include <stdexcept>

namespace utils::ebcdic {
namespace {

const std::vector<CCSIDMetadata> AvailableCcsids{
    {CCSID::Ccsid37, ccsids::Ccsid37Number, ccsids::Ccsid37Description},
};

} // namespace

const std::vector<CCSIDMetadata>& availableCcsids()
{
    return AvailableCcsids;
}

const CCSIDMetadata& ccsidMetadata(CCSID ccsid)
{
    for (const auto& metadata : AvailableCcsids) {
        if (metadata.ccsid == ccsid) {
            return metadata;
        }
    }

    throw std::invalid_argument("unknown CCSID");
}

std::string ccsidMenuLabel(CCSID ccsid)
{
    const auto& metadata = ccsidMetadata(ccsid);
    return std::to_string(metadata.number) + ": " + metadata.description;
}

const std::array<unsigned char, 256>& toLatin1Table(CCSID ccsid)
{
    switch (ccsid) {
    case CCSID::Ccsid37:
        return ccsids::ccsid37ToLatin1Table();
    }

    return ccsids::ccsid37ToLatin1Table();
}

} // namespace utils::ebcdic
