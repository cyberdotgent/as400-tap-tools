#pragma once

#include "RecordParser.h"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace tap {
class TapeImage;
}

namespace as400 {

struct FileListEntry {
    std::size_t element_index = 0;
    RecordInfo record;
    std::string file_name;
    std::string size;
    std::string set;
    std::string section;
    std::string sequence;
    std::string generation;
    std::string created;
    std::string expires;
    std::string system;
};

std::optional<FileListEntry> makeAs400FileListEntry(std::size_t element_index, const RecordInfo& record);

std::vector<FileListEntry> collectAs400FileList(
    const tap::TapeImage& image,
    const RecordParser& parser);

} // namespace as400
