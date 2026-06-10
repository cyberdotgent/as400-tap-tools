#include "App.h"

#include "MainFrame.h"

#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/string.h>
#include <wx/stdpaths.h>

wxIMPLEMENT_APP(TapeToolsApp);

namespace {

std::filesystem::path PathFromWxString(const wxString& value)
{
#if defined(_WIN32)
    return std::filesystem::path(value.ToStdWstring());
#else
    return std::filesystem::path(value.ToStdString());
#endif
}

wxString AppIconPath()
{
    const auto& standard_paths = wxStandardPaths::Get();
#if defined(__APPLE__)
    return wxFileName(standard_paths.GetResourcesDir(), "as400-tap-tools.png").GetFullPath();
#else
    wxFileName executable(standard_paths.GetExecutablePath());
    return wxFileName(executable.GetPath(), "assets/as400-tap-tools.png").GetFullPath();
#endif
}

} // namespace

bool TapeToolsApp::OnInit()
{
    if (!wxApp::OnInit()) {
        return false;
    }

    wxInitAllImageHandlers();

    auto* frame = new MainFrame(wxString::FromUTF8("AS400 Tape Tools"));
    const auto icon_path = AppIconPath();
    if (wxFileName::FileExists(icon_path)) {
        wxIcon icon(icon_path, wxBITMAP_TYPE_PNG);
        if (icon.IsOk()) {
            frame->SetIcon(icon);
        }
    }
    frame->Show(true);
    if (startup_tape_path_) {
        frame->LoadTapeFile(*startup_tape_path_);
    }
    return true;
}

void TapeToolsApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);
    parser.AddParam(
        wxString::FromUTF8("tape-file"),
        wxCMD_LINE_VAL_STRING,
        wxCMD_LINE_PARAM_OPTIONAL);
}

bool TapeToolsApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if (!wxApp::OnCmdLineParsed(parser)) {
        return false;
    }

    if (parser.GetParamCount() > 0) {
        startup_tape_path_ = PathFromWxString(parser.GetParam(0));
    } else {
        startup_tape_path_.reset();
    }

    return true;
}
