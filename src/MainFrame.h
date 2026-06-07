#pragma once

#include <wx/frame.h>

class wxCommandEvent;
class wxString;

class MainFrame final : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);

private:
    void BuildMenuBar();
    void BuildContent();
    void OnAbout(wxCommandEvent& event);
};
