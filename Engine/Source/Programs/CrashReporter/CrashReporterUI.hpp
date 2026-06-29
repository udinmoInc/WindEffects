#pragma once
#include "Core/Widget.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace we::programs::crashreporter {

class CrashReporterUI : public we::UI::Widget {
public:
    CrashReporterUI();
    ~CrashReporterUI() override = default;

    void Construct();
    void Paint(we::UI::PaintContext& context) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    we::UI::Size Measure(const we::UI::Size& availableSize) override;

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

    std::shared_ptr<we::UI::Widget> m_RootLayout;
};

} // namespace we::programs::crashreporter
