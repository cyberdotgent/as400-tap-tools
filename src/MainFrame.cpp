#include "MainFrame.h"

#include <wx/aboutdlg.h>
#include <wx/button.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

namespace {
constexpr int AboutMenuId = wxID_ABOUT;
constexpr int HelloButtonId = wxID_HIGHEST + 1;
}

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(720, 480))
{
    BuildMenuBar();
    BuildContent();

    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, AboutMenuId);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
    Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(
            wxString::FromUTF8("Hello from wxWidgets."),
            wxString::FromUTF8("SIMH Tape Tools"),
            wxOK | wxICON_INFORMATION);
    }, HelloButtonId);
}

void MainFrame::BuildMenuBar()
{
    auto* fileMenu = new wxMenu();
    fileMenu->Append(wxID_EXIT, wxString::FromUTF8("E&xit\tAlt-X"));

    auto* helpMenu = new wxMenu();
    helpMenu->Append(AboutMenuId, wxString::FromUTF8("&About"));

    auto* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, wxString::FromUTF8("&File"));
    menuBar->Append(helpMenu, wxString::FromUTF8("&Help"));
    SetMenuBar(menuBar);
}

void MainFrame::BuildContent()
{
    auto* panel = new wxPanel(this);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* heading = new wxStaticText(
        panel,
        wxID_ANY,
        wxString::FromUTF8("SIMH Tape Tools"));

    auto* message = new wxStaticText(
        panel,
        wxID_ANY,
        wxString::FromUTF8("Hello World. Tape file tools will be added here."));

    auto* button = new wxButton(
        panel,
        HelloButtonId,
        wxString::FromUTF8("Say Hello"));

    sizer->AddStretchSpacer(1);
    sizer->Add(heading, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 12);
    sizer->Add(message, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);
    sizer->Add(button, 0, wxALIGN_CENTER_HORIZONTAL);
    sizer->AddStretchSpacer(1);

    panel->SetSizer(sizer);
}

void MainFrame::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo info;
    info.SetName(wxString::FromUTF8("SIMH Tape Tools"));
    info.SetVersion(wxString::FromUTF8("0.1.0"));
    info.SetDescription(wxString::FromUTF8("Hello World wxWidgets shell for SIMH tape tooling."));
    wxAboutBox(info, this);
}
