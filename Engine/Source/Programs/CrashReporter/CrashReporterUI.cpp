#include "CrashReporterUI.h"
#include "Widgets/Panel.h"
#include "KindUI/Layout/Box.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/Button.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Widgets/CheckBox.h"
#include "Core/Logger.h"
#include "ConfigManager.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <windows.h>
#include <cstdlib>

using namespace we::runtime::kindui;

namespace we::programs::crashreporter {

CrashReporterUI::CrashReporterUI() {
    m_CrashDir = "Saved/Logs/Crashes/Latest";
}

void CrashReporterUI::LoadCrashData() {
    HE_INFO("[CrashReporterUI] LoadCrashData started");
    HE_INFO("[CrashReporterUI] Crash directory: " + m_CrashDir);
    
    // Check if crash directory exists
    if (!std::filesystem::exists(m_CrashDir)) {
        HE_WARN("[CrashReporterUI] Crash directory does not exist: " + m_CrashDir);
        m_CrashData = nlohmann::json::object();
        m_ExceptionData = nlohmann::json::object();
        m_SystemData = nlohmann::json::object();
        m_ModulesData = nlohmann::json::object();
        m_MemoryData = nlohmann::json::object();
        m_StackTrace = "No crash data available - Crash Reporter was opened manually.";
        m_EngineLog = "";
        HE_INFO("[CrashReporterUI] LoadCrashData completed (no crash data)");
        return;
    }

    HE_INFO("[CrashReporterUI] Crash directory exists, reading files");

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

    HE_INFO("[CrashReporterUI] Reading Crash.json");
    m_CrashData = read_json("Crash.json");
    HE_INFO("[CrashReporterUI] Reading Exception.json");
    m_ExceptionData = read_json("Exception.json");
    HE_INFO("[CrashReporterUI] Reading System.json");
    m_SystemData = read_json("System.json");
    HE_INFO("[CrashReporterUI] Reading Modules.json");
    m_ModulesData = read_json("Modules.json");
    HE_INFO("[CrashReporterUI] Reading Memory.json");
    m_MemoryData = read_json("Memory.json");

    HE_INFO("[CrashReporterUI] Reading StackTrace.txt");
    m_StackTrace = read_file(m_CrashDir + "/StackTrace.txt");
    HE_INFO("[CrashReporterUI] Reading Engine.log");
    m_EngineLog = read_file(m_CrashDir + "/Engine.log");
    
    HE_INFO("[CrashReporterUI] LoadCrashData completed");
}

void CrashReporterUI::Construct() {
    HE_INFO("[CrashReporterUI] Construct started");
    
    try {
        HE_INFO("[CrashReporterUI] Loading ConfigManager");
        ConfigManager::Get().Load();
        HE_INFO("[CrashReporterUI] ConfigManager loaded");
        
        LoadCrashData();
    } catch (const std::exception& e) {
        HE_ERROR("[CrashReporterUI] Failed to load crash reporter config or data: " + std::string(e.what()));
        // Set default values to prevent UI crash
        m_CrashData = nlohmann::json::object();
        m_ExceptionData = nlohmann::json::object();
        m_SystemData = nlohmann::json::object();
        m_ModulesData = nlohmann::json::object();
        m_MemoryData = nlohmann::json::object();
        m_StackTrace = "Error loading crash data: " + std::string(e.what());
        m_EngineLog = "";
    }

    HE_INFO("[CrashReporterUI] Building UI layout");
    auto rootBox = std::make_shared<VerticalBox>();
    rootBox->SetPadding(Margin{16.0f, 16.0f, 16.0f, 16.0f});
    rootBox->SetSpacing(12.0f);
    
    const auto& cfg = ConfigManager::Get().GetConfig();
    std::string projName = m_CrashData.value("Project", "WindEffectsProject");
    HE_INFO("[CrashReporterUI] Project name: " + projName);

    // 1. Header Label
    auto header = std::make_shared<Label>("An Unreal process has crashed: " + projName, Color{0.9f, 0.9f, 0.9f, 1.0f}, 18.0f);
    header->SetHorizontalAlignment(HorizontalAlignment::Left);
    rootBox->AddChild(header);

    // 2. Apology & Instructions
    std::string apology = "We are very sorry that this crash occurred. Our goal is to prevent crashes like this from occurring in the future. Please help us track down and fix this crash by providing detailed information about what you were doing so that we may reproduce the crash and fix it quickly. You can also log a Bug Report with us using the Bug Submission Form and work directly with support staff to report this issue.\n\nThanks for your help in improving the WindEffects Engine.";
    auto apologyLabel = std::make_shared<Label>(apology, Color{0.75f, 0.75f, 0.75f, 1.0f}, 12.0f);
    apologyLabel->SetHorizontalAlignment(HorizontalAlignment::Left);
    apologyLabel->SetWrapText(true);
    rootBox->AddChild(apologyLabel);

    // 3. User Input (Placeholder via Label for now)
    auto inputPanel = std::make_shared<VerticalBox>();
    inputPanel->SetPadding(Margin{8.0f, 8.0f, 8.0f, 8.0f});
    auto inputLabel = std::make_shared<Label>("Please provide detailed information about what you were doing when the crash occurred.", Color{0.5f, 0.5f, 0.5f, 1.0f}, 12.0f);
    inputLabel->SetHorizontalAlignment(HorizontalAlignment::Left);
    inputPanel->AddChild(inputLabel);
    rootBox->AddChild(inputPanel);

    // 4. Directory Link
    auto dirBox = std::make_shared<HorizontalBox>();
    dirBox->SetSpacing(4.0f);
    dirBox->SetHorizontalAlignment(HorizontalAlignment::Left);
    auto dirLabel1 = std::make_shared<Label>("Crash reports comprise diagnostics files (", Color{0.6f, 0.6f, 0.6f, 1.0f}, 12.0f);
    dirBox->AddChild(dirLabel1);
    dirBox->AddChild(std::make_shared<Button>("click here to view directory", [this]{ OnOpenCrashFolder(); }));
    auto dirLabel2 = std::make_shared<Label>(") and the following summary information:", Color{0.6f, 0.6f, 0.6f, 1.0f}, 12.0f);
    dirBox->AddChild(dirLabel2);
    rootBox->AddChild(dirBox);

    // 5. Diagnostic Summary (Scrollable Terminal)
    auto stackScroll = std::make_shared<ScrollLayout>();
    auto stackPanel = std::make_shared<VerticalBox>();
    stackPanel->SetPadding(Margin{8.0f, 8.0f, 8.0f, 8.0f});
    
    std::string summaryStr = "LoginId: " + m_SystemData.value("LoginId", "Unknown") + "\n";
    summaryStr += "EpicAccountId: " + m_SystemData.value("EpicAccountId", "Unknown") + "\n\n";
    summaryStr += "Caught signal\n\n";
    summaryStr += m_StackTrace.empty() ? "No stack trace available." : m_StackTrace;

    auto stackLabel = std::make_shared<Label>(summaryStr, Color{0.8f, 0.8f, 0.8f, 1.0f}, 11.0f);
    stackLabel->SetHorizontalAlignment(HorizontalAlignment::Left);
    stackPanel->AddChild(stackLabel);
    
    auto bgPanel = std::make_shared<Panel>();
    bgPanel->SetBackgroundColor(Color{0.05f, 0.05f, 0.05f, 1.0f});
    bgPanel->SetContent(stackPanel);
    stackScroll->SetContent(bgPanel);
    
    // Fill remaining space with the scroll layout
    rootBox->AddChild(stackScroll);

    // 6. Consent Checkboxes
    auto check1 = std::make_shared<CheckBox>("Include log files with submission. I understand that logs contain some personal information such as my system and user name.", true);
    check1->SetHorizontalAlignment(HorizontalAlignment::Left);
    rootBox->AddChild(check1);

    auto check2 = std::make_shared<CheckBox>("I agree to be contacted by Epic Games via email if additional information about this crash would help fix it.", true);
    check2->SetHorizontalAlignment(HorizontalAlignment::Left);
    rootBox->AddChild(check2);

    // 7. Bottom Action Bar
    auto bottomBar = std::make_shared<HorizontalBox>();
    bottomBar->AddChild(std::make_shared<Button>("Close Without Sending", [this]{ exit(0); }));
    bottomBar->AddChild(std::make_shared<Spacer>()); // Push rest to the right
    bottomBar->AddChild(std::make_shared<Button>("Send and Close", [this]{ OnExportZip(); exit(0); }));
    
    // Add small spacer between right buttons
    auto smallSpacer = std::make_shared<Spacer>();
    bottomBar->AddChild(smallSpacer);
    bottomBar->AddChild(std::make_shared<Button>("Send and Restart", [this]{ OnExportZip(); OnRestartEditor(); }));

    rootBox->AddChild(bottomBar);

    m_RootLayout = rootBox;
}

Size CrashReporterUI::Measure(const Size& availableSize) {
    if (m_RootLayout) m_RootLayout->Measure(availableSize);
    return availableSize;
}

void CrashReporterUI::Arrange(const Rect& allottedRect) {
    if (m_RootLayout) {
        m_RootLayout->Arrange(allottedRect);
    }
}

void CrashReporterUI::Paint(PaintContext& context) {
    if (m_RootLayout) {
        m_RootLayout->Paint(context);
    }
}

void CrashReporterUI::OnExportZip() {
    std::string cmd = "powershell.exe -c \"Compress-Archive -Path '" + m_CrashDir + "/All*' -DestinationPath 'CrashReport.zip' -Force\"";
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
