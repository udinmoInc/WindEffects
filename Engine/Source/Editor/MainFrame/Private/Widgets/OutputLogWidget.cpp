// ==============================================================================
// WindEffects — MainFrame — OutputLogWidget
// UI widget used by the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Widgets/OutputLogWidget.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Tokens/SurfaceRole.h"

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::SurfaceRole;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;

namespace we::editor::panels {

OutputLogWidget::OutputLogWidget() {
    for (const auto& record : we::Logger::GetHistory()) {
        m_Records.push_back(record);
    }
    RebuildVisibleLines();
}

OutputLogWidget::~OutputLogWidget() = default;

Size OutputLogWidget::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void OutputLogWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void OutputLogWidget::Tick(float /*deltaTime*/) {
    if (m_Paused) return;

    const auto newLogs = we::Logger::GetNewLogs();
    if (newLogs.empty()) return;

    {
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        for (const auto& record : newLogs) {
            m_Records.push_back(record);
        }
        while (m_Records.size() > kMaxStoredRecords) {
            m_Records.pop_front();
        }
    }
    RebuildVisibleLines();
}

void OutputLogWidget::OnMouseWheel(const ::we::runtime::kindui::MouseEvent& event) {
    const float rowH = ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
    const float delta = event.wheelDeltaY * rowH * 3.0f;
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const float contentHeight = static_cast<float>(m_VisibleLines.size()) * rowH;
    const float maxScroll = std::max(0.0f, contentHeight - m_Geometry.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset - delta, 0.0f, maxScroll);
    if (delta > 0.0f) {
        m_AutoScroll = false;
    }
}

void OutputLogWidget::Clear() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    m_Records.clear();
    m_VisibleLines.clear();
    m_VisibleLevels.clear();
    m_ScrollOffset = 0.0f;
    m_InfoCount = 0;
    m_WarningCount = 0;
    m_ErrorCount = 0;
    m_TotalCount = 0;
}

void OutputLogWidget::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    RebuildVisibleLines();
}

void OutputLogWidget::SetMinimumLevel(we::Logger::Level level) {
    m_MinLevel = level;
    RebuildVisibleLines();
}

void OutputLogWidget::SetCategoryFilter(const std::string& category) {
    m_CategoryFilter = category;
    RebuildVisibleLines();
}

Color OutputLogWidget::LevelColor(we::Logger::Level level) const {
    switch (level) {
        case we::Logger::Level::Trace: return ThemeColor(ColorToken::TextHint);
        case we::Logger::Level::Debug: return ThemeColor(ColorToken::TextSecondary);
        case we::Logger::Level::Info: return ThemeColor(ColorToken::TextPrimary);
        case we::Logger::Level::Warning: return ThemeColor(ColorToken::Warning);
        case we::Logger::Level::Error: return ThemeColor(ColorToken::ErrorForeground);
        case we::Logger::Level::Critical: return ThemeColor(ColorToken::ErrorForeground);
    }
    return ThemeColor(ColorToken::TextPrimary);
}

bool OutputLogWidget::PassesFilter(const we::Logger::LogRecord& record) const {
    if (static_cast<int>(record.level) < static_cast<int>(m_MinLevel)) return false;
    if (!m_CategoryFilter.empty() && record.category != m_CategoryFilter) return false;
    if (!m_SearchQuery.empty() && record.formattedText.find(m_SearchQuery) == std::string::npos) return false;
    return true;
}

void OutputLogWidget::RebuildVisibleLines() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    RebuildVisibleLinesUnlocked();
}

void OutputLogWidget::RebuildVisibleLinesUnlocked() {
    m_VisibleLines.clear();
    m_VisibleLevels.clear();
    m_InfoCount = 0;
    m_WarningCount = 0;
    m_ErrorCount = 0;
    m_TotalCount = m_Records.size();

    for (const auto& record : m_Records) {
        if (record.level == we::Logger::Level::Info || record.level == we::Logger::Level::Debug || record.level == we::Logger::Level::Trace) {
            m_InfoCount++;
        } else if (record.level == we::Logger::Level::Warning) {
            m_WarningCount++;
        } else if (record.level == we::Logger::Level::Error || record.level == we::Logger::Level::Critical) {
            m_ErrorCount++;
        }

        if (!PassesFilter(record)) continue;
        m_VisibleLines.push_back(record.formattedText);
        m_VisibleLevels.push_back(record.level);
    }
    if (m_AutoScroll) {
        const float contentHeight = static_cast<float>(m_VisibleLines.size()) * ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
        m_ScrollOffset = std::max(0.0f, contentHeight - m_Geometry.height);
    }
}

void OutputLogWidget::Paint(PaintContext& context) {
    if (!m_Visible) return;

    std::vector<std::string> visibleLines;
    std::vector<we::Logger::Level> visibleLevels;
    float scrollOffset = 0.0f;
    Rect geometry;
    {
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        visibleLines = m_VisibleLines;
        visibleLevels = m_VisibleLevels;
        scrollOffset = m_ScrollOffset;
        geometry = m_Geometry;
    }

    ::we::runtime::kindui::panels::PanelChrome::PaintContentRegion(context, geometry);

    const float lineHeight = ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
    float y = geometry.y - scrollOffset;
    const float maxY = geometry.y + geometry.height;

    for (size_t i = 0; i < visibleLines.size(); ++i) {
        if (y + lineHeight < geometry.y) {
            y += lineHeight;
            continue;
        }
        if (y > maxY) break;

        // Alternating row background accent
        if (i % 2 == 1) {
            context.DrawSurface(
                Rect{ geometry.x, y, geometry.width, lineHeight },
                SurfaceRole::Recessed,
                0.0f,
                "LogSubtleRow");
        }

        context.DrawText(
            visibleLines[i],
            Point{ geometry.x + ::we::runtime::kindui::panels::PanelChrome::PanelPaddingH(), y + (lineHeight - ThemeMetric(MetricToken::TextSizeCaption)) * 0.5f },
            LevelColor(visibleLevels[i]),
            ThemeMetric(MetricToken::TextSizeCaption));
        y += lineHeight;
    }
}

} // namespace we::editor::panels
