// ==============================================================================
// WindEffects — MainFrame — OutputLogWidget
// Public API surface for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "MainFrame/Export.h"

#include <KindUI/EditorUI.h>
#include "Core/Logger.h"
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace we::editor::panels {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::PaintContext;

class MAINFRAME_API OutputLogWidget : public Widget {
public:
    OutputLogWidget();
    ~OutputLogWidget() override;

    void Tick(float deltaTime) override;
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseWheel(const ::we::runtime::kindui::MouseEvent& event) override;

    void SetPaused(bool paused) { m_Paused = paused; }
    bool IsPaused() const { return m_Paused; }
    void SetAutoScroll(bool enabled) { m_AutoScroll = enabled; }
    bool IsAutoScroll() const { return m_AutoScroll; }
    void Clear();
    void SetSearchQuery(const std::string& query);
    void SetMinimumLevel(we::Logger::Level level);
    void SetCategoryFilter(const std::string& category);

    size_t GetInfoCount() const { return m_InfoCount; }
    size_t GetWarningCount() const { return m_WarningCount; }
    size_t GetErrorCount() const { return m_ErrorCount; }
    size_t GetTotalCount() const { return m_TotalCount; }

private:
    we::runtime::kindui::Color LevelColor(we::Logger::Level level) const;
    bool PassesFilter(const we::Logger::LogRecord& record) const;
    void RebuildVisibleLines();
    void RebuildVisibleLinesUnlocked();

    std::deque<we::Logger::LogRecord> m_Records;
    std::vector<std::string> m_VisibleLines;
    std::vector<we::Logger::Level> m_VisibleLevels;
    std::recursive_mutex m_Mutex;
    std::string m_SearchQuery;
    std::string m_CategoryFilter;
    we::Logger::Level m_MinLevel = we::Logger::Level::Trace;
    bool m_Paused = false;
    bool m_AutoScroll = true;
    float m_ScrollOffset = 0.0f;
    size_t m_InfoCount = 0;
    size_t m_WarningCount = 0;
    size_t m_ErrorCount = 0;
    size_t m_TotalCount = 0;
    static constexpr size_t kMaxStoredRecords = 5000;
};

} // namespace we::editor::panels
