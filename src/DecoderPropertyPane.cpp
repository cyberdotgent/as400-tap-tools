#include "DecoderPropertyPane.h"

#include <sstream>

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/treelist.h>
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

wxTreeListItem appendProperty(wxTreeListCtrl* tree, wxTreeListItem parent, const DecoderProperty& property)
{
    const auto item = tree->AppendItem(parent, Utf8(property.name));
    tree->SetItemText(item, 1, Utf8(property.value));

    for (const auto& child : property.children) {
        appendProperty(tree, item, child);
    }

    if (!property.children.empty()) {
        tree->Expand(item);
    }

    return item;
}

std::string itemPath(const wxTreeListCtrl* tree, wxTreeListItem item)
{
    std::vector<std::string> names;
    for (auto current = item; current.IsOk(); current = tree->GetItemParent(current)) {
        const auto name = tree->GetItemText(current, 0).ToStdString();
        if (!name.empty()) {
            names.push_back(name);
        }
    }

    std::ostringstream output;
    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        if (output.tellp() > 0) {
            output << '.';
        }
        output << *it;
    }
    return output.str();
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

    properties_ = new wxTreeListCtrl(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxTL_SINGLE);
    properties_->AppendColumn(wxString::FromUTF8("Property"), 180, wxALIGN_LEFT, wxCOL_RESIZABLE);
    properties_->AppendColumn(wxString::FromUTF8("Value"), 520, wxALIGN_LEFT, wxCOL_RESIZABLE);

    sizer->Add(title_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(properties_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizer(sizer);
}

void DecoderPropertyPane::SetTitle(std::string_view title)
{
    title_->SetValue(Utf8(title));
}

void DecoderPropertyPane::SetProperties(const std::vector<DecoderProperty>& properties)
{
    properties_->DeleteAllItems();
    const auto root = properties_->GetRootItem();
    for (const auto& property : properties) {
        appendProperty(properties_, root, property);
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
    wxTreeListItems selections;
    properties_->GetSelections(selections);
    for (const auto& item : selections) {
        if (!item.IsOk()) {
            continue;
        }
        if (output.tellp() > 0) {
            output << '\n';
        }
        output << itemPath(properties_, item)
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
