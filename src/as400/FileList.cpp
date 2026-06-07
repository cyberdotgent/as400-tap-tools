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

std::optional<FileListEntry> makeAs400FileListEntry(std::size_t element_index, const RecordInfo& record)
{
    if (!record.recognized || record.type != RecordType::Header1) {
        return std::nullopt;
    }

    FileListEntry entry;
    entry.element_index = element_index;
    entry.record = record;
    entry.file_name = fieldValue(record, "File");
    entry.set = fieldValue(record, "Set");
    entry.section = fieldValue(record, "Section");
    entry.sequence = fieldValue(record, "Sequence");
    entry.generation = fieldValue(record, "Generation");
    entry.created = fieldValue(record, "Created");
    entry.expires = fieldValue(record, "Expires");
    entry.system = fieldValue(record, "System");
    return entry;
}

std::vector<FileListEntry> collectAs400FileList(
    const tap::TapeImage& image,
    const RecordParser& parser)
{
    std::vector<FileListEntry> entries;
    const auto& elements = image.elements();
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto& element = elements[index];
        if (!element.isRecord()) {
            continue;
        }

        const auto record = parser.parseRecord(element.record().data);
        if (const auto entry = makeAs400FileListEntry(index, record)) {
            entries.push_back(std::move(*entry));
        }
    }

    return entries;
}

} // namespace as400
