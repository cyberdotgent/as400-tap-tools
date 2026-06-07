#include "App.h"

#include "MainFrame.h"

#include <wx/string.h>

wxIMPLEMENT_APP(TapeToolsApp);

bool TapeToolsApp::OnInit()
{
    if (!wxApp::OnInit()) {
        return false;
    }

    auto* frame = new MainFrame(wxString::FromUTF8("SIMH Tape Tools"));
    frame->Show(true);
    return true;
}
