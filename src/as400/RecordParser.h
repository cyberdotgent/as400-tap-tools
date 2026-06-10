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

struct RecordField {
    std::string name;
    std::vector<std::uint8_t> raw_value;
    std::string display_value;
    bool has_display_value = false;
    std::vector<RecordField> children;
};

struct RecordInfo {
    RecordType type = RecordType::Unknown;
    std::string code;
    std::string name;
    std::string details;
    std::vector<RecordField> fields;
    bool recognized = false;
};

class RecordParser {
public:
    RecordInfo parseRecord(const std::vector<std::uint8_t>& data) const;
    bool isAs400Tape(const tap::TapeImage& image) const;
};

} // namespace as400
