#include "TapeAnalysis.h"

#include "TapeElementView.h"

TapeAnalysisBuilder::TapeAnalysisBuilder(const as400::RecordParser& parser)
    : parser_(parser)
{
}

void TapeAnalysisBuilder::reserve(std::size_t total_elements)
{
    analysis_.element_views.reserve(total_elements);
    analysis_.parsed_records.reserve(total_elements);
}

void TapeAnalysisBuilder::observeElement(std::size_t index, const tap::TapeElement& element)
{
    analysis_.element_views.push_back(describeTapeElement(element));

    as400::RecordInfo record;
    if (element.isRecord()) {
        record = parser_.parseRecord(element.record().data);
        file_list_collector_.observe(index, record, element.record().data.size());

        if (!analysis_.volume_label && record.recognized && record.type == as400::RecordType::VolumeLabel) {
            analysis_.volume_label = record;
        }

        if (record.recognized) {
            ++recognized_count_;
            if (recognized_count_ == 1 && record.type == as400::RecordType::VolumeLabel) {
                first_record_is_volume_label_ = true;
            }
            if (record.type == as400::RecordType::Header1 || record.type == as400::RecordType::Header2) {
                has_header_ = true;
            }
            if (first_record_is_volume_label_ && (has_header_ || recognized_count_ >= 1)) {
                analysis_.is_as400_tape = true;
            }
        }
    }

    analysis_.parsed_records.push_back(std::move(record));
}

TapeAnalysis TapeAnalysisBuilder::finish()
{
    analysis_.file_list_entries = file_list_collector_.takeEntries();
    return std::move(analysis_);
}

TapeAnalysis analyzeTapeImage(const tap::TapeImage& image, const as400::RecordParser& parser)
{
    TapeAnalysisBuilder builder(parser);
    builder.reserve(image.elements().size());
    for (std::size_t index = 0; index < image.elements().size(); ++index) {
        builder.observeElement(index, image.elements()[index]);
    }
    return builder.finish();
}
