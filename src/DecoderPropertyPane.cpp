#include "DecoderPropertyPane.h"

#include <sstream>

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/window.h>

namespace {

wxString Utf8(std::string_view text)
{
    return wxString::FromUTF8(text.data(), text.size());
}

bool isDescendantOf(wxWindow* child, const wxWindow* parent)
{
    while (child != nullptr) {
        if (child == parent) {
            return true;
        }
        child = child->GetParent();
    }
    return false;
}

} // namespace

DecoderPropertyPane::DecoderPropertyPane(wxWindow* parent)
    : wxPanel(parent)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    title_ = new wxTextCtrl(
        this,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_READONLY | wxBORDER_NONE);

    properties_ = new wxListCtrl(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(-1, 116),
        wxLC_REPORT | wxLC_SINGLE_SEL);
    properties_->AppendColumn(wxString::FromUTF8("Property"), wxLIST_FORMAT_LEFT, 160);
    properties_->AppendColumn(wxString::FromUTF8("Value"), wxLIST_FORMAT_LEFT, 520);

    sizer->Add(title_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(properties_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizer(sizer);
}

void DecoderPropertyPane::SetTitle(std::string_view title)
{
    title_->SetValue(Utf8(title));
}

void DecoderPropertyPane::SetProperties(const std::vector<DecoderProperty>& properties)
{
    properties_->DeleteAllItems();
    for (std::size_t index = 0; index < properties.size(); ++index) {
        const auto row = properties_->InsertItem(static_cast<long>(index), Utf8(properties[index].name));
        properties_->SetItem(row, 1, Utf8(properties[index].value));
    }
}

void DecoderPropertyPane::SetMessage(std::string_view title, std::string_view message)
{
    SetTitle(title);
    SetProperties({DecoderProperty{"Status", std::string(message)}});
}

bool DecoderPropertyPane::HasFocusedChild() const
{
    return isDescendantOf(wxWindow::FindFocus(), this);
}

bool DecoderPropertyPane::CopySelectionToClipboard() const
{
    std::ostringstream output;
    long item = -1;
    while (true) {
        item = properties_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1) {
            break;
        }

        if (output.tellp() > 0) {
            output << '\n';
        }
        output << properties_->GetItemText(item).ToStdString()
               << '\t'
               << properties_->GetItemText(item, 1).ToStdString();
    }

    if (output.tellp() == 0) {
        return false;
    }

    if (!wxTheClipboard->Open()) {
        return false;
    }

    wxTheClipboard->SetData(new wxTextDataObject(Utf8(output.str())));
    wxTheClipboard->Close();
    return true;
}
