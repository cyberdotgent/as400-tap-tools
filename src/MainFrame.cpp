#include "MainFrame.h"

#include "RawTapeExplorerDialog.h"
#include "TapeAnalysis.h"
#include "TapeProgressDialog.h"
#include "utils/RecordFields.h"
#include "tap/Reader.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include <wx/aboutdlg.h>
#include <wx/event.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/font.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/statusbr.h>
#include <wx/string.h>
#include <wx/toolbar.h>

namespace {

constexpr int OpenMenuId = wxID_OPEN;
constexpr int CloseFileMenuId = wxID_CLOSE;
constexpr int ExitMenuId = wxID_EXIT;
constexpr int AboutMenuId = wxID_ABOUT;
constexpr int RawTapeExplorerMenuId = wxID_HIGHEST + 5;

wxString Utf8(std::string_view text)
{
    return wxString::FromUTF8(text.data(), text.size());
}

wxString AssetPath(std::string_view file_name)
{
    const auto& standard_paths = wxStandardPaths::Get();
#if defined(__APPLE__)
    return wxFileName(standard_paths.GetResourcesDir(), wxString::FromUTF8(file_name.data(), file_name.size())).GetFullPath();
#else
    wxFileName executable(standard_paths.GetExecutablePath());
    auto relative_path = std::string("assets/");
    relative_path.append(file_name);
    return wxFileName(executable.GetPath(), wxString::FromUTF8(relative_path.c_str())).GetFullPath();
#endif
}

wxBitmap LoadToolbarBitmap(std::string_view file_name)
{
#if defined(_WIN32)
    const auto resource_name =
        file_name == "toolbar-open.png"
            ? wxString::FromUTF8("TOOLBAR_OPEN")
            : wxString::FromUTF8("TOOLBAR_RAW_EXPLORER");
    wxBitmap bitmap(resource_name, wxBITMAP_TYPE_PNG_RESOURCE);
    if (!bitmap.IsOk()) {
        return {};
    }

    if (bitmap.GetWidth() == 24 && bitmap.GetHeight() == 24) {
        return bitmap;
    }

    auto image = bitmap.ConvertToImage();
    if (!image.IsOk()) {
        return bitmap;
    }

    image.Rescale(24, 24, wxIMAGE_QUALITY_HIGH);
    return wxBitmap(image);
#else
    const auto path = AssetPath(file_name);
    if (!wxFileName::FileExists(path)) {
        return {};
    }

    wxImage image(path, wxBITMAP_TYPE_PNG);
    if (!image.IsOk()) {
        return {};
    }

    image.Rescale(24, 24, wxIMAGE_QUALITY_HIGH);
    return wxBitmap(image);
#endif
}

template <typename T>
T progressStep(T total)
{
    return std::max<T>(1, total / 100);
}

template <typename T>
bool shouldUpdateProgress(T current, T total, T& last_bucket, T step)
{
    if (total == 0) {
        return true;
    }

    const auto bucket = current / step;
    const auto last = last_bucket / step;
    if (bucket != last || current == total) {
        last_bucket = current;
        return true;
    }

    return false;
}

} // namespace

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1100, 720))
{
    BuildMenuBar();
    BuildToolBar();
    BuildContent();
    CreateStatusBar();
    UpdateWindowTitle();
    UpdateStatusText();
    UpdateToolState();

    Bind(wxEVT_MENU, &MainFrame::OnOpen, this, OpenMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnCloseFile, this, CloseFileMenuId);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, ExitMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnRawExplorerView, this, RawTapeExplorerMenuId);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, AboutMenuId);
    file_list_view_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MainFrame::OnFileListItemActivated, this);
}

void MainFrame::BuildMenuBar()
{
    auto* file_menu = new wxMenu();
    file_menu->Append(OpenMenuId, wxString::FromUTF8("&Open...\tCtrl-O"));
    file_menu->Append(CloseFileMenuId, wxString::FromUTF8("&Close"));
    file_menu->AppendSeparator();
    file_menu->Append(ExitMenuId, wxString::FromUTF8("E&xit\tAlt-X"));

    auto* view_menu = new wxMenu();
    raw_explorer_item_ = view_menu->Append(RawTapeExplorerMenuId, wxString::FromUTF8("&Raw Tape Explorer...\tCtrl-R"));

    auto* help_menu = new wxMenu();
    help_menu->Append(AboutMenuId, wxString::FromUTF8("&About"));

    auto* menu_bar = new wxMenuBar();
    menu_bar->Append(file_menu, wxString::FromUTF8("&File"));
    menu_bar->Append(view_menu, wxString::FromUTF8("&View"));
    menu_bar->Append(help_menu, wxString::FromUTF8("&Help"));
    SetMenuBar(menu_bar);
}

void MainFrame::BuildToolBar()
{
    tool_bar_ = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL);
    if (!tool_bar_) {
        return;
    }

    tool_bar_->SetToolBitmapSize(wxSize(24, 24));
    tool_bar_->AddTool(OpenMenuId, wxString::FromUTF8("Open"), LoadToolbarBitmap("toolbar-open.png"), wxString::FromUTF8("Open tape"));
    tool_bar_->AddTool(
        RawTapeExplorerMenuId,
        wxString::FromUTF8("Raw Tape Explorer"),
        LoadToolbarBitmap("toolbar-raw-explorer.png"),
        wxString::FromUTF8("Open raw tape explorer"));
    tool_bar_->Realize();
}

void MainFrame::BuildContent()
{
    auto* panel = new wxPanel(this);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    BuildFileListView(panel);

    sizer->Add(file_list_panel_, 1, wxEXPAND);
    panel->SetSizer(sizer);
}

void MainFrame::BuildFileListView(wxWindow* parent)
{
    file_list_panel_ = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* header_sizer = new wxBoxSizer(wxHORIZONTAL);
    volume_label_caption_ = new wxStaticText(file_list_panel_, wxID_ANY, wxString::FromUTF8("Volume:"));
    volume_label_value_ = new wxStaticText(file_list_panel_, wxID_ANY, wxString::FromUTF8("-"));
    owner_label_caption_ = new wxStaticText(file_list_panel_, wxID_ANY, wxString::FromUTF8("Owner:"));
    owner_label_value_ = new wxStaticText(file_list_panel_, wxID_ANY, wxString::FromUTF8("-"));

    auto monospace_font = volume_label_value_->GetFont();
    monospace_font.SetFamily(wxFONTFAMILY_TELETYPE);
    volume_label_value_->SetFont(monospace_font);
    owner_label_value_->SetFont(monospace_font);

    header_sizer->Add(volume_label_caption_, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
    header_sizer->Add(volume_label_value_, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 20);
    header_sizer->Add(owner_label_caption_, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
    header_sizer->Add(owner_label_value_, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(header_sizer, 0, wxALL | wxEXPAND, 10);

    file_list_view_ = new wxListCtrl(file_list_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    file_list_view_->AppendColumn(wxString::FromUTF8("#"), wxLIST_FORMAT_RIGHT, 54);
    file_list_view_->AppendColumn(wxString::FromUTF8("File"), wxLIST_FORMAT_LEFT, 180);
    file_list_view_->AppendColumn(wxString::FromUTF8("Size"), wxLIST_FORMAT_RIGHT, 92);
    file_list_view_->AppendColumn(wxString::FromUTF8("Set"), wxLIST_FORMAT_LEFT, 96);
    file_list_view_->AppendColumn(wxString::FromUTF8("Section"), wxLIST_FORMAT_LEFT, 80);
    file_list_view_->AppendColumn(wxString::FromUTF8("Sequence"), wxLIST_FORMAT_LEFT, 84);
    file_list_view_->AppendColumn(wxString::FromUTF8("Generation"), wxLIST_FORMAT_LEFT, 92);
    file_list_view_->AppendColumn(wxString::FromUTF8("Created"), wxLIST_FORMAT_LEFT, 112);
    file_list_view_->AppendColumn(wxString::FromUTF8("Expires"), wxLIST_FORMAT_LEFT, 112);
    file_list_view_->AppendColumn(wxString::FromUTF8("System"), wxLIST_FORMAT_LEFT, 140);
    sizer->Add(file_list_view_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    file_list_panel_->SetSizer(sizer);
}

void MainFrame::LoadTapeFile(const std::filesystem::path& path)
{
    tap::Reader reader;
    TapeAnalysisBuilder analysis_builder(as400_parser_);
    const auto read_result = [&]() {
        TapeProgressDialog progress(this, "Loading tape file", "bytes");
        std::uint64_t last_progress_bytes = 0;
        std::uint64_t step = 1;
        std::uint64_t seen_total = 0;
        return reader.read(
            path,
            [&progress, &last_progress_bytes, &step, &seen_total](const tap::ProgressInfo& info) {
                if (seen_total != info.bytes_total) {
                    seen_total = info.bytes_total;
                    step = progressStep<std::uint64_t>(std::max<std::uint64_t>(1, info.bytes_total));
                }
                if (shouldUpdateProgress(info.bytes_read, info.bytes_total, last_progress_bytes, step)) {
                    progress.SetProgress("Reading and analyzing tape file...", info.bytes_read, info.bytes_total);
                }
            },
            [&analysis_builder](const tap::TapeElement& element, std::size_t index) {
                analysis_builder.observeElement(index, element);
            });
    }();
    if (!read_result) {
        const auto& error = read_result.error();
        wxMessageBox(
            wxString::Format(
                wxString::FromUTF8("Could not open tape file.\n\nOffset: %llu\nError: %s"),
                static_cast<unsigned long long>(error.offset),
                wxString::FromUTF8(error.message.c_str())),
            wxString::FromUTF8("Open Tape"),
            wxOK | wxICON_ERROR,
            this);
        return;
    }

    loaded_path_ = path;
    tape_analysis_ = analysis_builder.finish();
    PopulateFileListView();

    UpdateWindowTitle();
    ShowFileListLoadMessage();
    UpdateStatusText();
    UpdateToolState();
}

void MainFrame::ClearTape()
{
    loaded_path_.clear();
    tape_analysis_.reset();
    PopulateFileListView();
    UpdateWindowTitle();
    UpdateStatusText();
    UpdateToolState();
}

void MainFrame::PopulateFileListView()
{
    if (!file_list_view_) {
        return;
    }

    file_list_view_->DeleteAllItems();
    if (!tape_analysis_) {
        UpdateFileListHeader();
        return;
    }

    for (std::size_t index = 0; index < tape_analysis_->file_list_entries.size(); ++index) {
        const auto& entry = tape_analysis_->file_list_entries[index];
        const auto row = file_list_view_->InsertItem(static_cast<long>(index), wxString::Format("%zu", index + 1));
        file_list_view_->SetItem(row, 1, Utf8(entry.file_name));
        file_list_view_->SetItem(row, 2, Utf8(entry.size));
        file_list_view_->SetItem(row, 3, Utf8(entry.set));
        file_list_view_->SetItem(row, 4, Utf8(entry.section));
        file_list_view_->SetItem(row, 5, Utf8(entry.sequence));
        file_list_view_->SetItem(row, 6, Utf8(entry.generation));
        file_list_view_->SetItem(row, 7, Utf8(entry.created));
        file_list_view_->SetItem(row, 8, Utf8(entry.expires));
        file_list_view_->SetItem(row, 9, Utf8(entry.system));
        file_list_view_->SetItemData(row, static_cast<long>(entry.element_index));
    }

    for (int column = 0; column < file_list_view_->GetColumnCount(); ++column) {
        file_list_view_->SetColumnWidth(column, wxLIST_AUTOSIZE_USEHEADER);
    }
    UpdateFileListHeader();
}

void MainFrame::UpdateFileListHeader()
{
    if (!volume_label_value_ || !owner_label_value_) {
        return;
    }

    if (!tape_analysis_ || !tape_analysis_->volume_label) {
        volume_label_value_->SetLabel(wxString::FromUTF8("-"));
        owner_label_value_->SetLabel(wxString::FromUTF8("-"));
        return;
    }

    const auto volume = ::utils::fieldValue(*tape_analysis_->volume_label, "Volume");
    const auto owner = ::utils::fieldValue(*tape_analysis_->volume_label, "Owner");
    volume_label_value_->SetLabel(wxString::FromUTF8(volume.empty() ? "-" : volume.c_str()));
    owner_label_value_->SetLabel(wxString::FromUTF8(owner.empty() ? "-" : owner.c_str()));

    if (file_list_panel_) {
        file_list_panel_->Layout();
    }
}

void MainFrame::UpdateToolState()
{
    const auto has_tape = tape_analysis_.has_value();
    if (raw_explorer_item_) {
        raw_explorer_item_->Enable(has_tape);
    }
    if (tool_bar_) {
        tool_bar_->EnableTool(RawTapeExplorerMenuId, has_tape);
    }
}

void MainFrame::UpdateWindowTitle()
{
    if (loaded_path_.empty()) {
        SetTitle(wxString::FromUTF8("AS400 Tape Tools"));
        return;
    }

    SetTitle(wxString::Format(
        wxString::FromUTF8("AS400 Tape Tools - %s"),
        wxString::FromUTF8(loaded_path_.string().c_str())));
}

void MainFrame::ShowFileListLoadMessage()
{
    if (loaded_path_.empty()) {
        return;
    }

    if (!tape_analysis_ || !tape_analysis_->is_as400_tape) {
        wxMessageBox(
            wxString::FromUTF8("The loaded tape is not recognized as an IBM AS/400 tape.\n\nUse View > Raw Tape Explorer to inspect the raw structure."),
            wxString::FromUTF8("Tape Not Recognized"),
            wxOK | wxICON_WARNING,
            this);
        return;
    }

    if (tape_analysis_->file_list_entries.empty()) {
        wxMessageBox(
            wxString::FromUTF8("The loaded IBM AS/400 tape contains no files."),
            wxString::FromUTF8("No Files"),
            wxOK | wxICON_INFORMATION,
            this);
    }
}

void MainFrame::UpdateStatusText()
{
    if (loaded_path_.empty()) {
        SetStatusText(wxString::FromUTF8("No tape loaded"));
        return;
    }

    if (!tape_analysis_ || !tape_analysis_->is_as400_tape) {
        SetStatusText(wxString::FromUTF8("Not recognized as an IBM AS/400 tape | Switch to View > Raw Tape Explorer for structural inspection"));
        return;
    }

    if (tape_analysis_->file_list_entries.empty()) {
        SetStatusText(wxString::FromUTF8("IBM AS/400 tape | No files"));
        return;
    }

    SetStatusText(wxString::Format(
        wxString::FromUTF8("IBM AS/400 tape | %zu files"),
        tape_analysis_->file_list_entries.size()));
}

void MainFrame::OnOpen(wxCommandEvent&)
{
    wxFileDialog dialog(
        this,
        wxString::FromUTF8("Open AS400 Tape File"),
        wxString(),
        wxString(),
        wxString::FromUTF8("Tape files (*.tap;*.TAP)|*.tap;*.TAP|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    LoadTapeFile(std::filesystem::path(dialog.GetPath().ToStdString()));
}

void MainFrame::OnCloseFile(wxCommandEvent&)
{
    ClearTape();
}

void MainFrame::OnRawExplorerView(wxCommandEvent&)
{
    if (!tape_analysis_) {
        wxMessageBox(
            wxString::FromUTF8("Load a tape before opening the raw tape explorer."),
            wxString::FromUTF8("Raw Tape Explorer"),
            wxOK | wxICON_INFORMATION,
            this);
        return;
    }

    RawTapeExplorerDialog dialog(
        this,
        *tape_analysis_,
        0);
    dialog.ShowModal();
}

void MainFrame::OnFileListItemActivated(wxListEvent& event)
{
    const auto item = event.GetIndex();
    if (item == wxNOT_FOUND) {
        return;
    }

    RawTapeExplorerDialog dialog(
        this,
        *tape_analysis_,
        static_cast<std::size_t>(file_list_view_->GetItemData(item)));
    dialog.ShowModal();
}

void MainFrame::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo info;
    info.SetName(wxString::FromUTF8("AS400 Tape Tools"));
    info.SetVersion(wxString::FromUTF8("0.1.0"));
    info.SetDescription(wxString::FromUTF8("AS400 tape file browser and editing shell."));
    wxAboutBox(info, this);
}
