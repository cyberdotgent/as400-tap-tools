#include "HexFormatter.h"
#include "as400/Cp37.h"
#include "as400/FileList.h"
#include "as400/RecordParser.h"
#include "tap/Reader.h"
#include "tap/Scanner.h"
#include "tap/Writer.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string bytes(std::initializer_list<unsigned char> values)
{
    std::string result;
    result.reserve(values.size());
    for (const auto value : values) {
        result.push_back(static_cast<char>(value));
    }
    return result;
}

std::vector<std::uint8_t> cp37Label(std::initializer_list<std::pair<std::size_t, std::string>> fields)
{
    std::string label(80, ' ');
    for (const auto& field : fields) {
        label.replace(field.first, field.second.size(), field.second);
    }
    return as400::encodeCp37(label);
}

void testTapeImageEditing()
{
    tap::TapeImage image;
    assert(image.empty());

    image.appendRecord({0x01, 0x02});
    image.appendTapeMark();
    image.appendRecord({0x03});

    assert(image.elementCount() == 3);
    assert(image.recordCount() == 2);
    assert(image.tapeMarkCount() == 1);

    image.move(2, 0);
    assert(image.elements()[0].isRecord());
    assert(image.elements()[0].record().data[0] == 0x03);

    image.erase(2);
    assert(image.elementCount() == 2);
    assert(image.recordCount() == 2);
    assert(image.tapeMarkCount() == 0);
}

void testRoundTripWithOddRecord()
{
    tap::TapeImage image;
    image.appendRecord({0x41, 0x42, 0x43});
    image.appendTapeMark();
    image.appendEraseGap();
    image.appendEndOfMedium();

    std::ostringstream output(std::ios::binary);
    tap::Writer writer;
    const auto write_result = writer.write(image, output);
    assert(write_result);

    const auto encoded = output.str();
    const auto expected_prefix = bytes({
        0x03, 0x00, 0x00, 0x00,
        0x41, 0x42, 0x43, 0x00,
        0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFE, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
    });
    assert(encoded == expected_prefix);

    std::istringstream input(encoded, std::ios::binary);
    tap::Reader reader;
    auto read_result = reader.read(input);
    assert(read_result);

    const auto& round_trip = read_result.value();
    assert(round_trip.elementCount() == 4);
    assert(round_trip.recordCount() == 1);
    assert(round_trip.tapeMarkCount() == 1);
    assert(round_trip.elements()[0].record().data == std::vector<std::uint8_t>({0x41, 0x42, 0x43}));
    assert(round_trip.elements()[2].isEraseGap());
    assert(round_trip.elements()[3].isEndOfMedium());
}

void testMismatchedTrailerFailsStrictRead()
{
    const auto encoded = bytes({
        0x03, 0x00, 0x00, 0x00,
        0x41, 0x42, 0x43, 0x00,
        0x04, 0x00, 0x00, 0x00,
    });

    std::istringstream input(encoded, std::ios::binary);
    tap::Reader reader;
    const auto result = reader.read(input);
    assert(!result);
    assert(result.error().code == tap::ErrorCode::MismatchedRecordTrailer);
    assert(result.error().offset == 8);
}

void testSampleScannerReportsZuluScsiTailAndReaderAcceptsIt()
{
    const auto sample_path = std::filesystem::path(TAP_TEST_DATA_DIR) / "blank.tap";
    tap::Scanner scanner;
    const auto result = scanner.scan(sample_path);

    assert(result.image.recordCount() == 1);
    assert(result.image.tapeMarkCount() == 21);
    assert(!result.diagnostics.empty());
    assert(result.diagnostics.front().code == tap::ErrorCode::TrailingPartialRecord);
    assert(result.diagnostics.front().offset == 172);

    tap::Reader reader;
    const auto read_result = reader.read(sample_path);
    assert(read_result);
    assert(read_result.value().recordCount() == 1);
    assert(read_result.value().tapeMarkCount() == 21);

    tap::ReaderOptions strict_options;
    strict_options.allow_zuluscsi_trailing_partial_record = false;
    tap::Reader strict_reader(strict_options);
    const auto strict_read = strict_reader.read(sample_path);
    assert(!strict_read);
    assert(strict_read.error().code == tap::ErrorCode::TrailingPartialRecord);
}

void testReaderAcceptsAllExampleTapes()
{
    const auto examples_path = std::filesystem::path(TAP_TEST_DATA_DIR);
    const std::vector<std::string> examples{
        "Autocoder+IOCS.tap",
        "Autocoder.Weaver.tap",
        "BB-D782F-BE_VMS_3.5.tap",
        "Fortran_v3m0.tap",
        "Fortran_v3m4.tap",
        "blank.tap",
    };

    tap::Reader reader;
    for (const auto& example : examples) {
        const auto read_result = reader.read(examples_path / example);
        assert(read_result);
        assert(!read_result.value().empty());
    }
}

void testHexFormatterDecodesCp37AndHighlightsTextColumn()
{
    const std::vector<std::uint8_t> data{0xE5, 0xD6, 0xD3, 0xF1};
    const auto formatted = formatHexView(data, TextEncoding::EbcdicCp37);
    assert(formatted.find("E5 D6 D3 F1") != std::string::npos);
    assert(formatted.find("VOL1") != std::string::npos);

    const auto needle = encodeSearchText("VOL1", TextEncoding::EbcdicCp37);
    const auto result = findBytesInHexView(data, needle, TextEncoding::EbcdicCp37);
    assert(result.found);
    assert(formatted.substr(result.text_offset, result.text_length) == "VOL1");
}

void testAs400RecordParserIdentifiesLabels()
{
    as400::RecordParser parser;

    auto volume_label = as400::encodeCp37("VOL1TEST 0                         TESTNK");
    volume_label.resize(80, 0x40);
    const auto volume_info = parser.parseRecord(volume_label);
    assert(volume_info.recognized);
    assert(volume_info.type == as400::RecordType::VolumeLabel);
    assert(volume_info.code == "VOL1");
    assert(volume_info.details.find("Volume: TEST") != std::string::npos);
    assert(!volume_info.fields.empty());
    assert(volume_info.fields[0].name == "Record type");
    assert(volume_info.fields[0].value == "VOL1");

    const auto header1 = cp37Label({
        {0, "HDR1"},
        {4, "QFILEIML"},
        {21, "DV4R4"},
        {27, "0001"},
        {31, "0001"},
        {35, "0001"},
        {41, "026158"},
        {47, "99999"},
        {54, "001024"},
        {60, "IBMOS400"},
    });
    const auto header_info = parser.parseRecord(header1);
    assert(header_info.recognized);
    assert(header_info.type == as400::RecordType::Header1);
    assert(header_info.details.find("File: QFILEIML") != std::string::npos);
    assert(std::any_of(header_info.fields.begin(), header_info.fields.end(), [](const as400::RecordField& field) {
        return field.name == "File" && field.value.find("QFILEIML") == 0;
    }));
    const auto created = std::find_if(header_info.fields.begin(), header_info.fields.end(), [](const as400::RecordField& field) {
        return field.name == "Created";
    });
    assert(created != header_info.fields.end());
    assert(created->value == "026158");
    assert(std::any_of(created->children.begin(), created->children.end(), [](const as400::RecordField& field) {
        return field.name == "Year" && field.value == "2026";
    }));
    assert(std::any_of(created->children.begin(), created->children.end(), [](const as400::RecordField& field) {
        return field.name == "Month" && field.value == "6";
    }));
    assert(std::any_of(created->children.begin(), created->children.end(), [](const as400::RecordField& field) {
        return field.name == "Day" && field.value == "7";
    }));

    const auto expires = std::find_if(header_info.fields.begin(), header_info.fields.end(), [](const as400::RecordField& field) {
        return field.name == "Expires";
    });
    assert(expires != header_info.fields.end());
    assert(expires->value == "Does not expire");
}

void testAs400DetectorAcceptsBlankTape()
{
    const auto sample_path = std::filesystem::path(TAP_TEST_DATA_DIR) / "blank.tap";
    tap::Reader reader;
    const auto read_result = reader.read(sample_path);
    assert(read_result);

    as400::RecordParser parser;
    assert(parser.isAs400Tape(read_result.value()));

    tap::TapeImage generic;
    generic.appendRecord({0x41, 0x42, 0x43});
    assert(!parser.isAs400Tape(generic));
}

void testAs400DetectorAcceptsVolumeAndHeaderLabels()
{
    tap::TapeImage image;
    image.appendRecord(as400::encodeCp37("VOL1DV4R4 0"));
    image.appendRecord(as400::encodeCp37("HDR1QFILEIML        DV4R4 0001000100010026158 99999000000IBMOS400"));

    as400::RecordParser parser;
    assert(parser.isAs400Tape(image));
}

void testAs400FileListCollectsHeaderRecords()
{
    tap::TapeImage image;
    image.appendRecord(as400::encodeCp37("VOL1DV4R4 0"));
    image.appendRecord(cp37Label({
        {0, "HDR1"},
        {4, "QFILEIML"},
        {21, "DV4R4"},
        {27, "0001"},
        {31, "0001"},
        {35, "0001"},
        {41, "026158"},
        {47, "99999"},
        {54, "001024"},
        {60, "IBMOS400"},
    }));
    image.appendRecord(std::vector<std::uint8_t>(1024, 0x01));
    image.appendRecord(std::vector<std::uint8_t>(512, 0x02));
    image.appendRecord(as400::encodeCp37("EOF1"));
    image.appendRecord(as400::encodeCp37("EOF2"));
    image.appendTapeMark();

    as400::RecordParser parser;
    const auto files = as400::collectAs400FileList(image, parser);
    assert(files.size() == 1);
    assert(files[0].element_index == 1);
    assert(files[0].file_name == "QFILEIML");
    assert(files[0].size == "1.5 K");
    assert(files[0].created == "2026-06-07");
    assert(files[0].expires == "Does not expire");
}

void testAs400FileListFormatsLargerSizes()
{
    as400::RecordInfo record;
    record.recognized = true;
    record.type = as400::RecordType::Header1;
    record.fields = {
        {"File", "BIGFILE", {}},
        {"Created", "026158", {{"Date", "2026-06-07", {}}}},
        {"Expires", "never", {{"Date", "2026-12-31", {}}}},
    };

    const auto entry = as400::makeAs400FileListEntry(7, record, 1073741824ULL);
    assert(entry);
    assert(entry->size == "1.0 G");
    assert(entry->created == "2026-06-07");
    assert(entry->expires == "Does not expire");
}

} // namespace

int main()
{
    testTapeImageEditing();
    testRoundTripWithOddRecord();
    testMismatchedTrailerFailsStrictRead();
    testSampleScannerReportsZuluScsiTailAndReaderAcceptsIt();
    testReaderAcceptsAllExampleTapes();
    testHexFormatterDecodesCp37AndHighlightsTextColumn();
    testAs400RecordParserIdentifiesLabels();
    testAs400DetectorAcceptsBlankTape();
    testAs400DetectorAcceptsVolumeAndHeaderLabels();
    testAs400FileListCollectsHeaderRecords();
    testAs400FileListFormatsLargerSizes();
    return 0;
}
