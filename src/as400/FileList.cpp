#include "FileList.h"

#include "utils/Format.h"
#include "utils/RecordFields.h"

namespace as400 {

std::optional<FileListEntry> makeAs400FileListEntry(
    std::size_t element_index,
    const RecordInfo& record,
    std::uint64_t payload_size_bytes)
{
    if (!record.recognized || record.type != RecordType::Header1) {
        return std::nullopt;
    }

    FileListEntry entry;
    entry.element_index = element_index;
    entry.record = record;
    entry.file_name = utils::fieldValue(record, "File");
    entry.size = utils::Format::humanSize(payload_size_bytes);
    entry.set = utils::fieldValue(record, "Set");
    entry.section = utils::fieldValue(record, "Section");
    entry.sequence = utils::fieldValue(record, "Sequence");
    entry.generation = utils::fieldValue(record, "Generation");
    entry.created = utils::decodedDateValue(record, "Created", false);
    entry.expires = utils::decodedDateValue(record, "Expires", true);
    entry.system = utils::fieldValue(record, "System");
    return entry;
}

void FileListCollector::observe(std::size_t element_index, const RecordInfo& record, std::uint64_t payload_size_bytes)
{
    if (record.recognized && record.type == RecordType::Header1) {
        finalizeOpenFile();
        open_file_ = OpenFile{element_index, record, 0};
        return;
    }

    if (!open_file_) {
        return;
    }

    if (record.recognized) {
        if (record.type == RecordType::EndOfFile1 || record.type == RecordType::EndOfFile2) {
            finalizeOpenFile();
            return;
        }
        if (record.type != RecordType::Unknown) {
            return;
        }
    }

    open_file_->payload_size_bytes += payload_size_bytes;
}

void FileListCollector::finish()
{
    finalizeOpenFile();
}

const std::vector<FileListEntry>& FileListCollector::entries() const
{
    return entries_;
}

std::vector<FileListEntry> FileListCollector::takeEntries()
{
    finish();
    return std::move(entries_);
}

void FileListCollector::finalizeOpenFile()
{
    if (!open_file_) {
        return;
    }

    if (const auto entry = makeAs400FileListEntry(open_file_->element_index, open_file_->record, open_file_->payload_size_bytes)) {
        entries_.push_back(std::move(*entry));
    }
    open_file_.reset();
}

std::vector<FileListEntry> collectAs400FileList(
    const tap::TapeImage& image,
    const RecordParser& parser)
{
    FileListCollector collector;
    const auto& elements = image.elements();
    for (std::size_t index = 0; index < elements.size(); ++index) {
        const auto& element = elements[index];
        if (!element.isRecord()) {
            continue;
        }

        const auto record = parser.parseRecord(element.record().data);
        collector.observe(index, record, element.record().data.size());
    }

    return collector.takeEntries();
}

} // namespace as400
