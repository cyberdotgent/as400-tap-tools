#pragma once

#include "HexFormatter.h"
#include "as400/RecordParser.h"
#include "tap/TapeImage.h"

#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#include <wx/frame.h>

class wxCommandEvent;
class wxListEvent;
class wxListCtrl;
class wxPanel;
class wxString;
class wxTextCtrl;

enum class DecoderMode {
    Generic,
    IbmAs400
};

class MainFrame final : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);
    void LoadTapeFile(const std::filesystem::path& path);

private:
    void BuildMenuBar();
    void BuildContent();

    void ClearTape();
    void PopulateStructureList();
    void ShowSelectedElement(std::size_t index);
    void RefreshHexView();
    void SetEncoding(TextEncoding encoding);
    void SetDecoderMode(DecoderMode decoder_mode);
    void DetectDecoderMode();
    void UpdateDecoderPanel();
    void UpdateWindowTitle();
    void UpdateStatusText();

    void OnOpen(wxCommandEvent& event);
    void OnCloseFile(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnEncodingAscii(wxCommandEvent& event);
    void OnEncodingEbcdic(wxCommandEvent& event);
    void OnDecoderGeneric(wxCommandEvent& event);
    void OnDecoderIbmAs400(wxCommandEvent& event);
    void OnStructureSelected(wxListEvent& event);
    void OnAbout(wxCommandEvent& event);

    wxPanel* right_panel_ = nullptr;
    wxPanel* decoder_panel_ = nullptr;
    wxTextCtrl* decoder_title_ = nullptr;
    wxTextCtrl* decoder_details_ = nullptr;
    wxListCtrl* structure_list_ = nullptr;
    wxTextCtrl* hex_view_ = nullptr;
    wxMenuItem* ascii_encoding_item_ = nullptr;
    wxMenuItem* ebcdic_encoding_item_ = nullptr;
    wxMenuItem* generic_decoder_item_ = nullptr;
    wxMenuItem* as400_decoder_item_ = nullptr;

    as400::RecordParser as400_parser_;
    tap::TapeImage tape_image_;
    std::filesystem::path loaded_path_;
    std::vector<std::uint8_t> selected_bytes_;
    std::size_t selected_element_index_ = std::numeric_limits<std::size_t>::max();
    TextEncoding encoding_ = TextEncoding::Ascii;
    DecoderMode decoder_mode_ = DecoderMode::Generic;
    std::string last_search_text_;
    std::size_t next_search_offset_ = 0;
};
