#pragma once

#include <cstddef>
#include <string_view>

#include <wx/dialog.h>

class wxGauge;
class wxStaticText;
class wxWindow;
class wxCloseEvent;

namespace tap {
struct ProgressInfo;
}

class TapeProgressDialog final : public wxDialog
{
public:
    explicit TapeProgressDialog(wxWindow* parent, std::string_view title);

    void Pulse(std::string_view activity, std::size_t current);
    void SetProgress(std::string_view activity, std::size_t current, std::size_t total);

private:
    void RefreshText(std::string_view activity, std::size_t current, std::size_t total, bool indeterminate);
    void PumpEvents();
    void OnClose(wxCloseEvent& event);

    wxStaticText* activity_label_ = nullptr;
    wxStaticText* count_label_ = nullptr;
    wxGauge* gauge_ = nullptr;
};
