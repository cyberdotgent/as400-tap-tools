#pragma once

#include "TapeAnalysis.h"
#include "as400/RecordParser.h"
#include "tap/TapeImage.h"
#include "utils/ebcdic/Ccsids.h"

#include <filesystem>
#include <optional>
#include <unordered_map>

#include <wx/frame.h>

class wxCommandEvent;
class wxListEvent;
class wxListCtrl;
class wxPanel;
class wxStaticText;
class wxString;
class wxToolBar;
class wxWindow;

class MainFrame final : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);
    void LoadTapeFile(const std::filesystem::path& path);
    void RefreshDisplayedData();

private:
    void BuildMenuBar();
    void BuildToolBar();
    void BuildContent();
    void BuildFileListView(wxWindow* parent);
    void BuildCcsidMenu(class wxMenu* view_menu);

    void ClearTape();
    void PopulateFileListView();
    void UpdateFileListHeader();
    void UpdateToolState();
    void UpdateCcsidMenuState();
    void ShowFileListLoadMessage();
    void UpdateWindowTitle();
    void UpdateStatusText();

    void OnOpen(wxCommandEvent& event);
    void OnCloseFile(wxCommandEvent& event);
    void OnRawExplorerView(wxCommandEvent& event);
    void OnFileListItemActivated(wxListEvent& event);
    void OnSelectCcsid(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    wxPanel* file_list_panel_ = nullptr;
    wxStaticText* volume_label_caption_ = nullptr;
    wxStaticText* volume_label_value_ = nullptr;
    wxStaticText* owner_label_caption_ = nullptr;
    wxStaticText* owner_label_value_ = nullptr;
    wxListCtrl* file_list_view_ = nullptr;
    wxToolBar* tool_bar_ = nullptr;
    class wxMenu* ccsid_menu_ = nullptr;
    wxMenuItem* raw_explorer_item_ = nullptr;

    as400::RecordParser as400_parser_;
    std::filesystem::path loaded_path_;
    std::optional<tap::TapeImage> tape_image_;
    std::optional<TapeAnalysis> tape_analysis_;
    utils::ebcdic::CCSID selected_ccsid_ = utils::ebcdic::CCSID::Ccsid37;
    std::unordered_map<int, utils::ebcdic::CCSID> ccsid_menu_ids_;
};
