#pragma once

#include "RecordParser.h"
#include <cstddef>
#include <cstdint>
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
    std::string size;
};

class FileListCollector {
public:
    void observe(std::size_t element_index, const RecordInfo& record, std::uint64_t payload_size_bytes);
    void finish();

    const std::vector<FileListEntry>& entries() const;
    std::vector<FileListEntry> takeEntries();

private:
    struct OpenFile {
        std::size_t element_index = 0;
        RecordInfo record;
        std::uint64_t payload_size_bytes = 0;
    };

    void finalizeOpenFile();

    std::optional<OpenFile> open_file_;
    std::vector<FileListEntry> entries_;
};

std::optional<FileListEntry> makeAs400FileListEntry(
    std::size_t element_index,
    const RecordInfo& record,
    std::uint64_t payload_size_bytes);

std::vector<FileListEntry> collectAs400FileList(
    const tap::TapeImage& image,
    const RecordParser& parser);

} // namespace as400
