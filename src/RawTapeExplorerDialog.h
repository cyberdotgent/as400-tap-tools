#pragma once

#include "DecoderPropertyPane.h"
#include "HexFormatter.h"
#include "TapeAnalysis.h"

#include <cstddef>

#include <wx/dialog.h>

class wxListCtrl;
class wxListEvent;
class wxSplitterWindow;
class wxTextCtrl;

class RawTapeExplorerDialog final : public wxDialog
{
public:
    RawTapeExplorerDialog(
        wxWindow* parent,
        const TapeAnalysis& tape_analysis,
        std::size_t initial_element_index = 0);

private:
    enum class DecoderMode {
        Generic,
        IbmAs400
    };

    void BuildContent();
    void PopulateStructureList();
    void ShowSelectedElement(std::size_t index);
    void RefreshHexView();
    void SetEncoding(TextEncoding encoding);
    void SetDecoderMode(DecoderMode decoder_mode);
    void DetectDecoderMode();
    void UpdateDecoderPanel();

    void OnStructureSelected(wxListEvent& event);

    const TapeAnalysis& tape_analysis_;
    wxSplitterWindow* main_splitter_ = nullptr;
    wxSplitterWindow* hex_decoder_splitter_ = nullptr;
    DecoderPropertyPane* decoder_panel_ = nullptr;
    wxListCtrl* structure_list_ = nullptr;
    wxTextCtrl* hex_view_ = nullptr;
    std::vector<std::uint8_t> selected_bytes_;
    std::size_t selected_element_index_ = static_cast<std::size_t>(-1);
    TextEncoding encoding_ = TextEncoding::Ascii;
    DecoderMode decoder_mode_ = DecoderMode::Generic;
};
