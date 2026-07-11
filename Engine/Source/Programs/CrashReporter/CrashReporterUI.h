#pragma once
#include "Core/Widget.h"
#include <string>
#include <vector>
#include <nlohmann/json.h>

namespace we::programs::crashreporter {

class CrashReporterUI : public WindEffects::Editor::UI::Widget {
public:
    CrashReporterUI();
    ~CrashReporterUI() override = default;

    void Construct();
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;

private:
    void LoadCrashData();
    void OnExportZip();
    void OnRestartEditor();
    void OnOpenCrashFolder();

    nlohmann::json m_CrashData;
    nlohmann::json m_ExceptionData;
    nlohmann::json m_SystemData;
    nlohmann::json m_ModulesData;
    nlohmann::json m_MemoryData;
    std::string m_StackTrace;
    std::string m_EngineLog;
    std::string m_CrashDir;

    // State for UI
    bool m_IncludeLogs = true;
    bool m_IncludeDump = true;
    bool m_IncludeScreenshot = true;
    bool m_IncludeSystemInfo = true;
    std::string m_UserComments;

    std::shared_ptr<WindEffects::Editor::UI::Widget> m_RootLayout;
};

} // namespace we::programs::crashreporter
