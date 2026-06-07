#include "TapeElementView.h"

#include <cstddef>
#include <iomanip>
#include <sstream>

namespace {

std::string formatOffset(std::uint64_t offset)
{
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << offset;
    return output.str();
}

std::string formatSize(std::size_t size)
{
    std::ostringstream output;
    output << size << " bytes";
    return output.str();
}

std::string recordClassName(tap::RecordClass record_class)
{
    switch (record_class) {
    case tap::RecordClass::Good:
        return "good";
    case tap::RecordClass::Bad:
        return "bad";
    case tap::RecordClass::TapeDescription:
        return "tape description";
    }

    return "unknown";
}

TapeElementView describeRecord(const tap::Record& record)
{
    TapeElementView view;
    view.type = "File / Record";
    view.offset = formatOffset(record.offset);
    view.bytes = record.data;
    view.size = formatSize(view.bytes.size());
    view.details = recordClassName(record.record_class) + " record, encoded length 0x";

    std::ostringstream encoded_length;
    encoded_length << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << record.encoded_length;
    view.details += encoded_length.str();
    return view;
}

} // namespace

TapeElementView describeTapeElement(const tap::TapeElement& element)
{
    if (element.isRecord()) {
        return describeRecord(element.record());
    }

    TapeElementView view;
    if (element.isTapeMark()) {
        view.type = "Tape Mark";
        view.offset = formatOffset(element.tapeMark().offset);
        view.details = "File separator marker";
        view.bytes = {0x00, 0x00, 0x00, 0x00};
    } else if (element.isEraseGap()) {
        view.type = "Erase Gap";
        view.offset = formatOffset(element.eraseGap().offset);
        view.details = "SIMH erase gap marker";
        view.bytes = {0xFE, 0xFF, 0xFF, 0xFF};
    } else if (element.isEndOfMedium()) {
        view.type = "End of Medium";
        view.offset = formatOffset(element.endOfMedium().offset);
        view.details = "SIMH end-of-medium marker";
        view.bytes = {0xFF, 0xFF, 0xFF, 0xFF};
    } else {
        view.type = "Unknown";
    }

    view.size = formatSize(view.bytes.size());
    return view;
}
