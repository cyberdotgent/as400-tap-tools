#pragma once

#include "TapeElementView.h"
#include "as400/FileList.h"
#include "as400/RecordParser.h"
#include "tap/TapeImage.h"

#include <optional>
#include <vector>

struct TapeAnalysis {
    std::vector<TapeElementView> element_views;
    std::vector<as400::RecordInfo> parsed_records;
    std::optional<as400::RecordInfo> volume_label;
    std::vector<as400::FileListEntry> file_list_entries;
    bool is_as400_tape = false;
};

class TapeAnalysisBuilder {
public:
    explicit TapeAnalysisBuilder(const as400::RecordParser& parser);

    void reserve(std::size_t total_elements);
    void observeElement(std::size_t index, const tap::TapeElement& element);
    TapeAnalysis finish();

private:
    const as400::RecordParser& parser_;
    TapeAnalysis analysis_;
    as400::FileListCollector file_list_collector_;
    std::size_t recognized_count_ = 0;
    bool first_record_is_volume_label_ = false;
    bool has_header_ = false;
};

TapeAnalysis analyzeTapeImage(const tap::TapeImage& image, const as400::RecordParser& parser);
