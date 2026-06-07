#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <wx/panel.h>

class wxTextCtrl;
class wxTreeListCtrl;
class wxWindow;

struct DecoderProperty {
    std::string name;
    std::string value;
    std::vector<DecoderProperty> children;
};

class DecoderPropertyPane final : public wxPanel
{
public:
    explicit DecoderPropertyPane(wxWindow* parent);

    void SetTitle(std::string_view title);
    void SetProperties(const std::vector<DecoderProperty>& properties);
    void SetMessage(std::string_view title, std::string_view message);

    bool HasFocusedChild() const;
    bool CopySelectionToClipboard() const;

private:
    wxTextCtrl* title_ = nullptr;
    wxTreeListCtrl* properties_ = nullptr;
};
