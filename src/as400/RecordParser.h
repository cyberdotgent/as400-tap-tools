#pragma once

#include "tap/TapeImage.h"

#include <cstdint>
#include <string>
#include <vector>

namespace as400 {

enum class RecordType {
    Unknown,
    VolumeLabel,
    Header1,
    Header2,
    EndOfFile1,
    EndOfFile2,
    UserHeader1,
    UserHeader2
};

struct RecordInfo {
    RecordType type = RecordType::Unknown;
    std::string code;
    std::string name;
    std::string details;
    bool recognized = false;
};

class RecordParser {
public:
    RecordInfo parseRecord(const std::vector<std::uint8_t>& data) const;
    bool isAs400Tape(const tap::TapeImage& image) const;
};

} // namespace as400
