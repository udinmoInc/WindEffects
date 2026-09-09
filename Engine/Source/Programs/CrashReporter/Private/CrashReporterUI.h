// ==============================================================================
// WindEffects — CrashReporter — CrashReporterUI
// Internal implementation for the CrashReporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once
#include "KindUI/Core/Widget.h"
#include <string>
#include <vector>
#include <nlohmann/json.h>

namespace we::programs::crashreporter {

class CrashReporterUI : public we::runtime::kindui::Widget {
public:
    CrashReporterUI();
    ~CrashReporterUI() override = default;

    void Construct();
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;

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

    std::shared_ptr<we::runtime::kindui::Widget> m_RootLayout;
};

} // namespace we::programs::crashreporter
