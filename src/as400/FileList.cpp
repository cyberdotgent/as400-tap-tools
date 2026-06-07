#include "FileList.h"

#include <algorithm>
#include <string_view>
#include <utility>

namespace as400 {
namespace {

std::string fieldValue(const RecordInfo& record, std::string_view name)
{
    const auto field = std::find_if(record.fields.begin(), record.fields.end(), [name](const RecordField& entry) {
        return entry.name == name;
    });
    if (field == record.fields.end()) {
        return {};
    }
    return field->value;
}

} // namespace

std::vector<FileListEntry> collectAs400FileList(
    const tap::TapeImage& image,
    const RecordParser& parser,
    const tap::ProgressCallback& progress)
{
    std::vector<FileListEntry> entries;
    const auto& elements = image.elements();
    std::size_t processed = 0;
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto& element = elements[index];
        ++processed;
        if (progress) {
            progress(tap::ProgressInfo{processed, elements.size(), 0, false});
        }
        if (!element.isRecord()) {
            continue;
        }

        const auto record = parser.parseRecord(element.record().data);
        if (!record.recognized || record.type != RecordType::Header1) {
            continue;
        }

        FileListEntry entry;
        entry.element_index = index;
        entry.record = record;
        entry.file_name = fieldValue(record, "File");
        entry.set = fieldValue(record, "Set");
        entry.section = fieldValue(record, "Section");
        entry.sequence = fieldValue(record, "Sequence");
        entry.generation = fieldValue(record, "Generation");
        entry.created = fieldValue(record, "Created");
        entry.expires = fieldValue(record, "Expires");
        entry.system = fieldValue(record, "System");
        entries.push_back(std::move(entry));
    }

    return entries;
}

} // namespace as400
