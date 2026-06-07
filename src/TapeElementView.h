#pragma once

#include "tap/TapeElement.h"

#include <cstdint>
#include <string>
#include <vector>

struct TapeElementView {
    std::string type;
    std::string offset;
    std::string size;
    std::string details;
    std::vector<std::uint8_t> bytes;
};

TapeElementView describeTapeElement(const tap::TapeElement& element);
