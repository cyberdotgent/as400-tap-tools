#pragma once

#include "as400/FileList.h"
#include "as400/RecordParser.h"
#include "tap/TapeImage.h"

#include <filesystem>
#include <optional>
#include <vector>

#include <wx/frame.h>

class wxCommandEvent;
class wxListEvent;
class wxListCtrl;
class wxPanel;
class wxStaticText;
class wxString;
class wxWindow;

class MainFrame final : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);
    void LoadTapeFile(const std::filesystem::path& path);

private:
    void BuildMenuBar();
    void BuildContent();
    void BuildFileListView(wxWindow* parent);

    void ClearTape();
    void PopulateFileListView();
    void CollectFileListData();
    void UpdateFileListHeader();
    void ShowFileListLoadMessage();
    void UpdateWindowTitle();
    void UpdateStatusText();

    void OnOpen(wxCommandEvent& event);
    void OnCloseFile(wxCommandEvent& event);
    void OnRawExplorerView(wxCommandEvent& event);
    void OnFileListItemActivated(wxListEvent& event);
    void OnAbout(wxCommandEvent& event);

    wxPanel* file_list_panel_ = nullptr;
    wxStaticText* volume_label_caption_ = nullptr;
    wxStaticText* volume_label_value_ = nullptr;
    wxStaticText* owner_label_caption_ = nullptr;
    wxStaticText* owner_label_value_ = nullptr;
    wxListCtrl* file_list_view_ = nullptr;
    wxMenuItem* raw_explorer_item_ = nullptr;

    as400::RecordParser as400_parser_;
    tap::TapeImage tape_image_;
    std::filesystem::path loaded_path_;
    std::optional<as400::RecordInfo> volume_label_;
    std::vector<as400::FileListEntry> file_list_entries_;
};
