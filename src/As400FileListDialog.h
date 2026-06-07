#pragma once

#include "as400/FileList.h"

#include <cstddef>
#include <vector>

#include <wx/dialog.h>

class wxListCtrl;
class wxListEvent;
class wxWindow;

class As400FileListDialog final : public wxDialog
{
public:
    As400FileListDialog(wxWindow* parent, const std::vector<as400::FileListEntry>& entries);

    bool HasSelection() const;
    std::size_t SelectedElementIndex() const;

private:
    void Populate(const std::vector<as400::FileListEntry>& entries);
    void OnItemActivated(wxListEvent& event);

    wxListCtrl* file_list_ = nullptr;
    std::size_t selected_element_index_ = static_cast<std::size_t>(-1);
};
