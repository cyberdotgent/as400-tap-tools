#pragma once

#include <filesystem>
#include <optional>

#include <wx/app.h>

class wxCmdLineParser;

class TapeToolsApp final : public wxApp
{
public:
    bool OnInit() override;
    void OnInitCmdLine(wxCmdLineParser& parser) override;
    bool OnCmdLineParsed(wxCmdLineParser& parser) override;

private:
    std::optional<std::filesystem::path> startup_tape_path_;
};
