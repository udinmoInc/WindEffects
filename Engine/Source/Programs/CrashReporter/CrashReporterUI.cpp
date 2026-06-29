#include "CrashReporterUI.hpp"
#include "Widgets/Panel.hpp"
#include "Layout/Box.hpp"
#include "Layout/Splitter.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/TextBox.hpp"
#include "Core/Logger.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <windows.h>
#include <cstdlib>

using namespace we::UI;

namespace we::programs::crashreporter {

CrashReporterUI::CrashReporterUI() {
    m_CrashDir = "Saved/Logs/Crashes/Latest";
}

void CrashReporterUI::LoadCrashData() {
    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream f(path);
        if (!f.is_open()) return "";
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };

    auto read_json = [&](const std::string& name) -> nlohmann::json {
        std::string content = read_file(m_CrashDir + "/" + name);
        if (content.empty()) return nlohmann::json::object();
        try {
            return nlohmann::json::parse(content);
        } catch(...) {
            return nlohmann::json::object();
        }
    };

    m_CrashData = read_json("Crash.json");
    m_ExceptionData = read_json("Exception.json");
    m_SystemData = read_json("System.json");
    m_ModulesData = read_json("Modules.json");
    m_MemoryData = read_json("Memory.json");

    m_StackTrace = read_file(m_CrashDir + "/StackTrace.txt");
    m_EngineLog = read_file(m_CrashDir + "/Engine.log");
}

void CrashReporterUI::Construct() {
    LoadCrashData();

    auto rootBox = std::make_shared<VerticalBox>();
    rootBox->SetPadding(Margin{20.0f, 20.0f, 20.0f, 20.0f});
    rootBox->SetSpacing(15.0f);
    
    // Header
    auto headerPanel = std::make_shared<Panel>("WindEffects Engine Crash Reporter");
    headerPanel->SetHeaderHeight(40.0f);

    auto mainContent = std::make_shared<VerticalBox>();
    mainContent->SetSpacing(10.0f);

    // Pretext
    mainContent->AddChild(std::make_shared<Label>("We're sorry, the WindEffects Engine has crashed.", Color{1.0f, 0.4f, 0.4f, 1.0f}, 24.0f));
    mainContent->AddChild(std::make_shared<Label>("A crash report has been generated to help us fix the issue.", Color::White(), 16.0f));
    mainContent->AddChild(std::make_shared<Label>(" ", Color::White(), 16.0f)); // spacer

    // Crash Details
    mainContent->AddChild(std::make_shared<Label>("Exception: " + m_ExceptionData.value("Message", "Unknown Exception"), Color{1.0f, 0.2f, 0.2f, 1.0f}, 18.0f));
    mainContent->AddChild(std::make_shared<Label>("Code: " + m_ExceptionData.value("Code", "N/A")));
    mainContent->AddChild(std::make_shared<Label>("Module: " + m_ExceptionData.value("Module", "N/A")));
    mainContent->AddChild(std::make_shared<Label>("Address: " + m_ExceptionData.value("Address", "N/A")));
    mainContent->AddChild(std::make_shared<Label>(" ", Color::White(), 16.0f));

    mainContent->AddChild(std::make_shared<Label>("Call Stack:", Color::White(), 16.0f));
    mainContent->AddChild(std::make_shared<Label>(m_StackTrace, Color{0.8f, 0.8f, 0.8f, 1.0f}, 12.0f));
    mainContent->AddChild(std::make_shared<Label>(" ", Color::White(), 16.0f));

    // Email form
    std::string supportEmail = "support@windeffects.com";
    std::string moduleName = m_ExceptionData.value("Module", "WindEffects");
    std::filesystem::path configPath = "Config/" + moduleName + ".json";
    
    if (std::filesystem::exists(configPath)) {
        try {
            std::ifstream f(configPath);
            nlohmann::json cfg = nlohmann::json::parse(f);
            if (cfg.contains("SupportEmail")) {
                supportEmail = cfg["SupportEmail"].get<std::string>();
            }
        } catch(...) {}
    }

    auto emailBox = std::make_shared<HorizontalBox>();
    emailBox->SetSpacing(10.0f);
    emailBox->AddChild(std::make_shared<Label>("Send report to: " + supportEmail, Color::White(), 14.0f));
    
    // Actions
    auto actionBox = std::make_shared<HorizontalBox>();
    actionBox->SetSpacing(20.0f);
    actionBox->AddChild(std::make_shared<Button>("Send Report", []{ /* TODO: Send Email logic */ }));
    actionBox->AddChild(std::make_shared<Button>("Export Zip", [this]{ OnExportZip(); }));
    actionBox->AddChild(std::make_shared<Button>("Open Crash Folder", [this]{ OnOpenCrashFolder(); }));
    actionBox->AddChild(std::make_shared<Button>("Restart Editor", [this]{ OnRestartEditor(); }));

    mainContent->AddChild(emailBox);
    mainContent->AddChild(actionBox);

    headerPanel->SetContent(mainContent);
    rootBox->AddChild(headerPanel);

    m_RootLayout = rootBox;
}

we::UI::Size CrashReporterUI::Measure(const Size& availableSize) {
    if (m_RootLayout) m_RootLayout->Measure(availableSize);
    return availableSize;
}

void CrashReporterUI::Arrange(const Rect& allottedRect) {
    if (m_RootLayout) m_RootLayout->Arrange(allottedRect);
}

void CrashReporterUI::Paint(PaintContext& context) {
    if (m_RootLayout) m_RootLayout->Paint(context);
}

void CrashReporterUI::OnExportZip() {
    std::string cmd = "powershell.exe -c \"Compress-Archive -Path '" + m_CrashDir + "/*' -DestinationPath 'CrashReport.zip' -Force\"";
    system(cmd.c_str());
}

void CrashReporterUI::OnRestartEditor() {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string editorPath = "WindEffectsEditor.exe";
    CreateProcessA(nullptr, (LPSTR)editorPath.c_str(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi);
    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    exit(0);
}

void CrashReporterUI::OnOpenCrashFolder() {
    std::string cmd = "explorer.exe \"" + m_CrashDir + "\"";
    system(cmd.c_str());
}

} // namespace we::programs::crashreporter
