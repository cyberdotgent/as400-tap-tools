#include "App.h"

#include "MainFrame.h"

#include <wx/cmdline.h>
#include <wx/string.h>

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

} // namespace

bool TapeToolsApp::OnInit()
{
    if (!wxApp::OnInit()) {
        return false;
    }

    auto* frame = new MainFrame(wxString::FromUTF8("AS400 Tape Tools"));
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
