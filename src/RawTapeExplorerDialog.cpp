#include "RawTapeExplorerDialog.h"

#include "utils/RecordFields.h"

#include <string_view>
#include <utility>

#include <wx/event.h>
#include <wx/font.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/string.h>
#include <wx/textctrl.h>

namespace {

constexpr long NoSelection = -1;

wxString Utf8(std::string_view text)
{
    return wxString::FromUTF8(text.data(), text.size());
}

DecoderProperty ToDecoderProperty(const as400::RecordField& field, utils::ebcdic::CCSID ccsid)
{
    DecoderProperty property;
    property.name = field.name;
    property.value = utils::renderFieldValue(field, ccsid);
    property.children.reserve(field.children.size());
    for (const auto& child : field.children) {
        property.children.push_back(ToDecoderProperty(child, ccsid));
    }
    return property;
}

} // namespace

RawTapeExplorerDialog::RawTapeExplorerDialog(
    wxWindow* parent,
    const tap::TapeImage& tape_image,
    const TapeAnalysis& tape_analysis,
    utils::ebcdic::CCSID& selected_ccsid,
    std::size_t initial_element_index)
    : wxDialog(parent,
          wxID_ANY,
          wxString::FromUTF8("Raw Tape Explorer"),
          wxDefaultPosition,
          wxSize(1100, 720),
      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX),
      tape_image_(tape_image),
      tape_analysis_(tape_analysis),
      selected_ccsid_(selected_ccsid)
{
    BuildContent();
    DetectDecoderMode();
    PopulateStructureList();

    if (!tape_analysis_.element_views.empty() && initial_element_index < tape_analysis_.element_views.size()) {
        structure_list_->SetItemState(static_cast<long>(initial_element_index), wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
        structure_list_->EnsureVisible(static_cast<long>(initial_element_index));
        ShowSelectedElement(initial_element_index);
    } else {
        RefreshHexView();
        UpdateDecoderPanel();
    }

    structure_list_->Bind(wxEVT_LIST_ITEM_SELECTED, &RawTapeExplorerDialog::OnStructureSelected, this);
    CentreOnParent();
}

void RawTapeExplorerDialog::BuildContent()
{
    auto* panel = new wxPanel(this);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    main_splitter_ = new wxSplitterWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    structure_list_ = new wxListCtrl(main_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    structure_list_->AppendColumn(wxString::FromUTF8("#"), wxLIST_FORMAT_RIGHT, 56);
    structure_list_->AppendColumn(wxString::FromUTF8("Type"), wxLIST_FORMAT_LEFT, 140);
    structure_list_->AppendColumn(wxString::FromUTF8("Offset"), wxLIST_FORMAT_LEFT, 110);
    structure_list_->AppendColumn(wxString::FromUTF8("Size"), wxLIST_FORMAT_LEFT, 110);
    structure_list_->AppendColumn(wxString::FromUTF8("Details"), wxLIST_FORMAT_LEFT, 220);

    hex_decoder_splitter_ = new wxSplitterWindow(main_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    hex_view_ = new wxTextCtrl(
        hex_decoder_splitter_,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
    hex_view_->SetFont(wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE)));

    decoder_panel_ = new DecoderPropertyPane(hex_decoder_splitter_);
    decoder_panel_->SetMessage("AS/400 record", "Select a data record to inspect its AS/400 label type.");
    hex_decoder_splitter_->Initialize(hex_view_);
    hex_decoder_splitter_->SetMinimumPaneSize(120);
    hex_decoder_splitter_->SetSashGravity(1.0);

    main_splitter_->SplitVertically(structure_list_, hex_decoder_splitter_, 420);
    main_splitter_->SetMinimumPaneSize(240);
    main_splitter_->SetSashGravity(0.0);

    sizer->Add(main_splitter_, 1, wxEXPAND);
    panel->SetSizer(sizer);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(panel, 1, wxEXPAND);
    SetSizer(root);
}

void RawTapeExplorerDialog::PopulateStructureList()
{
    structure_list_->DeleteAllItems();

    if (tape_analysis_.element_views.empty()) {
        return;
    }
    for (std::size_t index = 0; index < tape_analysis_.element_views.size(); ++index) {
        const auto& view = tape_analysis_.element_views[index];
        const auto row = structure_list_->InsertItem(static_cast<long>(index), wxString::Format("%zu", index));
        structure_list_->SetItem(row, 1, Utf8(view.type));
        structure_list_->SetItem(row, 2, Utf8(view.offset));
        structure_list_->SetItem(row, 3, Utf8(view.size));
        structure_list_->SetItem(row, 4, Utf8(view.details));
        structure_list_->SetItemData(row, static_cast<long>(index));
    }
}

void RawTapeExplorerDialog::ShowSelectedElement(std::size_t index)
{
    if (index >= tape_analysis_.element_views.size()) {
        selected_bytes_.clear();
        selected_element_index_ = static_cast<std::size_t>(-1);
    } else {
        selected_bytes_ = tape_analysis_.element_views[index].bytes;
        selected_element_index_ = index;
    }

    RefreshHexView();
    UpdateDecoderPanel();
}

void RawTapeExplorerDialog::RefreshHexView()
{
    if (!hex_view_) {
        return;
    }

    hex_view_->SetValue(Utf8(formatHexView(selected_bytes_, encoding_, selected_ccsid_)));
}

void RawTapeExplorerDialog::SetEncoding(TextEncoding encoding)
{
    encoding_ = encoding;
    RefreshHexView();
    UpdateDecoderPanel();
}

void RawTapeExplorerDialog::SetDecoderMode(DecoderMode decoder_mode)
{
    decoder_mode_ = decoder_mode;
    if (decoder_mode_ == DecoderMode::IbmAs400) {
        SetEncoding(TextEncoding::Ebcdic);
        if (hex_decoder_splitter_ && !hex_decoder_splitter_->IsSplit()) {
            const auto height = std::max(120, hex_decoder_splitter_->GetClientSize().GetHeight() - 180);
            hex_decoder_splitter_->SplitHorizontally(hex_view_, decoder_panel_, height);
        }
    } else {
        SetEncoding(TextEncoding::Ascii);
        if (hex_decoder_splitter_ && hex_decoder_splitter_->IsSplit()) {
            hex_decoder_splitter_->Unsplit(decoder_panel_);
        }
    }

    if (hex_decoder_splitter_) {
        hex_decoder_splitter_->Layout();
    }
    UpdateDecoderPanel();
}

void RawTapeExplorerDialog::DetectDecoderMode()
{
    SetDecoderMode(tape_analysis_.is_as400_tape ? DecoderMode::IbmAs400 : DecoderMode::Generic);
}

void RawTapeExplorerDialog::UpdateDecoderPanel()
{
    if (!decoder_panel_ || decoder_mode_ != DecoderMode::IbmAs400) {
        return;
    }

    if (selected_element_index_ >= tape_analysis_.parsed_records.size()) {
        decoder_panel_->SetMessage("AS/400 record", "Select a data record to inspect its AS/400 label type.");
        hex_decoder_splitter_->Layout();
        return;
    }

    const auto& info = tape_analysis_.parsed_records[selected_element_index_];
    if (info.recognized) {
        DecoderProperty record;
        record.name = "Record";
        record.value = info.name;
        record.children.push_back(DecoderProperty{"Label code", info.code, {}});
        record.children.push_back(DecoderProperty{"Decoder", "IBM AS/400", {}});
        for (const auto& field : info.fields) {
            record.children.push_back(ToDecoderProperty(field, selected_ccsid_));
        }
        if (info.fields.empty()) {
            record.children.push_back(DecoderProperty{"Status", "No additional label fields decoded.", {}});
        }
        decoder_panel_->SetTitle("AS/400 record: " + info.name + " (" + info.code + ")");
        decoder_panel_->SetProperties({record});
    } else {
        decoder_panel_->SetTitle("AS/400 record: Unknown");
        decoder_panel_->SetProperties({
            DecoderProperty{
                "Record",
                "Unknown",
                {
                    DecoderProperty{"Status", "No AS/400 tape label recognized.", {}},
                    DecoderProperty{"Leading CP37 text", info.code, {}},
                },
            },
        });
    }

    hex_decoder_splitter_->Layout();
}

void RawTapeExplorerDialog::OnStructureSelected(wxListEvent& event)
{
    const auto item = event.GetIndex();
    if (item == NoSelection) {
        return;
    }

    ShowSelectedElement(static_cast<std::size_t>(structure_list_->GetItemData(item)));
}
