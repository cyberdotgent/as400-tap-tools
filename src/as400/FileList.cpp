#include "FileList.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
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

std::string decodedDateValue(const RecordInfo& record, std::string_view name, bool expiration_date)
{
    const auto field = std::find_if(record.fields.begin(), record.fields.end(), [name](const RecordField& entry) {
        return entry.name == name;
    });
    if (field == record.fields.end()) {
        return {};
    }

    if (expiration_date) {
        if (field->value == "Does not expire") {
            return field->value;
        }
        std::string lower_value = field->value;
        std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (lower_value == "never") {
            return "Does not expire";
        }
    }

    const auto decoded = std::find_if(field->children.begin(), field->children.end(), [](const RecordField& child) {
        return child.name == "Date";
    });
    if (decoded != field->children.end() && !decoded->value.empty()) {
        return decoded->value;
    }

    return field->value;
}

std::string formatHumanSize(std::uint64_t value)
{
    static constexpr const char* suffixes[] = {"B", "K", "M", "G"};
    static constexpr std::size_t suffix_count = sizeof(suffixes) / sizeof(suffixes[0]);
    double scaled = static_cast<double>(value);
    std::size_t suffix_index = 0;
    while (scaled >= 1024.0 && suffix_index + 1 < suffix_count) {
        scaled /= 1024.0;
        ++suffix_index;
    }

    std::ostringstream output;
    if (suffix_index == 0) {
        output << value << ' ' << suffixes[suffix_index];
    } else {
        output << std::fixed << std::setprecision(1) << scaled << ' ' << suffixes[suffix_index];
    }
    return output.str();
}

std::string formattedSizeValue(const RecordInfo& record)
{
    const auto field = std::find_if(record.fields.begin(), record.fields.end(), [](const RecordField& entry) {
        return entry.name == "Block count";
    });
    if (field == record.fields.end() || field->value.empty()) {
        return {};
    }
    if (!std::all_of(field->value.begin(), field->value.end(), [](unsigned char ch) {
            return std::isdigit(static_cast<int>(ch)) != 0;
        })) {
        return field->value;
    }

    return formatHumanSize(static_cast<std::uint64_t>(std::stoull(field->value)));
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
    entry.size = formattedSizeValue(record);
    entry.set = fieldValue(record, "Set");
    entry.section = fieldValue(record, "Section");
    entry.sequence = fieldValue(record, "Sequence");
    entry.generation = fieldValue(record, "Generation");
    entry.created = decodedDateValue(record, "Created", false);
    entry.expires = decodedDateValue(record, "Expires", true);
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
