// ==============================================================================
// WindEffects — MainFrame — OutputLogWidget
// UI widget used by the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Widgets/OutputLogWidget.h"
#include <KindUI/EditorUI.h>
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::SurfaceRole;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

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
    InvalidatePaint();
}

void OutputLogWidget::OnMouseWheel(const ::we::runtime::kindui::MouseEvent& event) {
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float rowH = ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const float contentHeight = static_cast<float>(m_VisibleRecords.size()) * rowH;
    const float bodyHeight = std::max(0.0f, m_Geometry.height - 20.0f * scale);
    m_Scroll.ApplyWheel(event.wheelDeltaY, rowH * 3.0f, bodyHeight, contentHeight);
    if (event.wheelDeltaY > 0.0f) {
        m_AutoScroll = false;
    }
    InvalidatePaint();
}

void OutputLogWidget::OnMouseDown(const ::we::runtime::kindui::MouseEvent& event) {
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float rowH = ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const float contentHeight = static_cast<float>(m_VisibleRecords.size()) * rowH;
    const float bodyHeight = std::max(0.0f, m_Geometry.height - 20.0f * scale);
    if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, bodyHeight, contentHeight)) {
        m_AutoScroll = false;
        InvalidatePaint();
    }
}

void OutputLogWidget::OnMouseMove(const ::we::runtime::kindui::MouseEvent& event) {
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float rowH = ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    const float contentHeight = static_cast<float>(m_VisibleRecords.size()) * rowH;
    const float bodyHeight = std::max(0.0f, m_Geometry.height - 20.0f * scale);
    const bool wasHovered = m_Scroll.IsThumbHovered();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, bodyHeight, contentHeight);
    if (m_Scroll.IsDraggingThumb() || m_Scroll.IsThumbHovered() != wasHovered) {
        if (m_Scroll.IsDraggingThumb()) {
            m_AutoScroll = false;
        }
        InvalidatePaint();
    }
}

void OutputLogWidget::OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) {
    m_Scroll.OnMouseUp(event);
    InvalidatePaint();
}

void OutputLogWidget::OnHoverLost() {
    m_Scroll.OnMouseMove(::we::runtime::kindui::MouseEvent{}, m_ScrollMetrics, 0.0f, 0.0f);
    InvalidatePaint();
}

bool OutputLogWidget::ShowsPointerCursor(const ::we::runtime::kindui::Point& position) const {
    return ::we::runtime::kindui::ScrollViewport::ShowsScrollbarCursor(m_ScrollMetrics, position);
}

void OutputLogWidget::Clear() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    m_Records.clear();
    m_VisibleRecords.clear();
    m_Scroll.offset = 0.0f;
    m_InfoCount = 0;
    m_WarningCount = 0;
    m_ErrorCount = 0;
    m_TotalCount = 0;
    InvalidatePaint();
}

void OutputLogWidget::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    RebuildVisibleLines();
    InvalidatePaint();
}

void OutputLogWidget::SetMinimumLevel(we::Logger::Level level) {
    m_MinLevel = level;
    RebuildVisibleLines();
    InvalidatePaint();
}

void OutputLogWidget::SetCategoryFilter(const std::string& category) {
    m_CategoryFilter = category;
    RebuildVisibleLines();
    InvalidatePaint();
}

Color OutputLogWidget::GetRecordColor(const we::Logger::LogRecord& record) const {
    if (record.category == "Cmd" || record.category == "Command" || record.category == "Exec" ||
        record.message.rfind(">", 0) == 0 || record.message.find("Cmd:") != std::string::npos || record.message.find("Command:") != std::string::npos ||
        record.formattedText.rfind(">", 0) == 0 || record.formattedText.find("Cmd:") != std::string::npos) {
        return Color{ 0.45f, 0.75f, 0.52f, 1.0f }; // Soft theme-harmonized emerald green for user commands
    }
    return LevelColor(record.level);
}

Color OutputLogWidget::LevelColor(we::Logger::Level level) const {
    switch (level) {
        case we::Logger::Level::Trace: return ThemeColor(ColorToken::TextPrimary);
        case we::Logger::Level::Debug: return ThemeColor(ColorToken::TextPrimary);
        case we::Logger::Level::Info: return ThemeColor(ColorToken::TextPrimary);
        case we::Logger::Level::Warning: return Color{ 0.88f, 0.70f, 0.32f, 1.0f }; // Soft theme amber/gold
        case we::Logger::Level::Error: return Color{ 0.85f, 0.42f, 0.42f, 1.0f };   // Soft theme muted coral/red
        case we::Logger::Level::Critical: return Color{ 0.85f, 0.42f, 0.42f, 1.0f };// Soft theme muted coral/red
    }
    return ThemeColor(ColorToken::TextPrimary);
}

bool OutputLogWidget::PassesFilter(const we::Logger::LogRecord& record) const {
    if (static_cast<int>(record.level) < static_cast<int>(m_MinLevel)) return false;
    if (!m_CategoryFilter.empty() && record.category != m_CategoryFilter) return false;
    if (!m_SearchQuery.empty() && record.formattedText.find(m_SearchQuery) == std::string::npos && record.message.find(m_SearchQuery) == std::string::npos) return false;
    return true;
}

void OutputLogWidget::RebuildVisibleLines() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    RebuildVisibleLinesUnlocked();
}

void OutputLogWidget::RebuildVisibleLinesUnlocked() {
    m_VisibleRecords.clear();
    m_InfoCount = 0;
    m_WarningCount = 0;
    m_ErrorCount = 0;
    m_TotalCount = m_Records.size();

    for (const auto& record : m_Records) {
        if (record.level == we::Logger::Level::Info || record.level == we::Logger::Level::Debug || record.level ==
            we::Logger::Level::Trace) {
            m_InfoCount++;
        } else if (record.level == we::Logger::Level::Warning) {
            m_WarningCount++;
        } else if (record.level == we::Logger::Level::Error || record.level == we::Logger::Level::Critical) {
            m_ErrorCount++;
        }

        if (!PassesFilter(record)) continue;
        m_VisibleRecords.push_back(record);
    }
    if (m_AutoScroll) {
        const float scale = (std::max)(1.0f, DPIContext::GetScale());
        const float contentHeight = static_cast<float>(m_VisibleRecords.size()) *
            ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
        const float bodyHeight = std::max(0.0f, m_Geometry.height - 20.0f * scale);
        m_Scroll.offset = std::max(0.0f, contentHeight - bodyHeight);
    }
}

void OutputLogWidget::Paint(PaintContext& context) {
    if (!m_Visible) return;

    std::vector<we::Logger::LogRecord> visibleRecords;
    Rect rawGeometry;
    {
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        visibleRecords = m_VisibleRecords;
        rawGeometry = m_Geometry;
    }

    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float marginH = 4.0f * scale;
    const float marginV = 2.0f * scale;
    const float paddingV = 8.0f * scale;

    const Rect geometry{
        rawGeometry.x + marginH,
        rawGeometry.y + marginV,
        std::max(0.0f, rawGeometry.width - marginH * 2.0f),
        std::max(0.0f, rawGeometry.height - marginV * 2.0f)
    };

    // 1. Recessed background surface for entire log container
    context.DrawSurface(geometry, SurfaceRole::Recessed, 3.0f * scale, "OutputLogRecessedBody");

    const float paddingH = 8.0f * scale;
    const float fontSize = ThemeMetric(MetricToken::TextSizeCaption);

    const float bodyYStart = geometry.y + paddingV;
    const float bodyHeight = std::max(0.0f, geometry.height - paddingV * 2.0f);

    // Update ScrollViewport metrics
    const float rowH = ::we::runtime::kindui::panels::PanelChrome::ListRowHeight();
    const float contentHeight = static_cast<float>(visibleRecords.size()) * rowH;
    const Rect fullBodyRect{ geometry.x, bodyYStart, geometry.width, bodyHeight };
    m_ScrollMetrics = m_Scroll.UpdateMetrics(fullBodyRect, bodyHeight, contentHeight, scale);

    // Viewport rect for content area (adjusts automatically for reserved scrollbar)
    const Rect contentViewport = m_ScrollMetrics.viewport;

    // Message Column takes full row width
    const float colMsgX = contentViewport.x + paddingH;
    const float colMsgW = std::max(0.0f, contentViewport.width - paddingH * 2.0f);

    // 2. Paint Log Body Rows
    context.PushClipRect(contentViewport);

    float y = bodyYStart - m_Scroll.offset;
    const float maxY = bodyYStart + bodyHeight;

    const Color textPrimary = ThemeColor(ColorToken::TextPrimary);

    for (size_t i = 0; i < visibleRecords.size(); ++i) {
        if (y + rowH < bodyYStart) {
            y += rowH;
            continue;
        }
        if (y > maxY) break;

        const Rect fullRowRect{ geometry.x, y, geometry.width, rowH };
        ::we::runtime::kindui::panels::PanelChrome::PaintAlternatingListRowBackground(context, fullRowRect, static_cast<int>(i));

        const auto& rec = visibleRecords[i];
        const float textY = y + (rowH - fontSize) * 0.5f - 1.0f * scale;

        // Log Message taking full row width with level/command-based color highlighting
        const Color msgColor = GetRecordColor(rec);
        const std::string& msgStr = !rec.formattedText.empty() ? rec.formattedText : rec.message;

        context.PushClipRect(Rect{ colMsgX, y, colMsgW, rowH });
        context.DrawText(msgStr, Point{ colMsgX, textY }, msgColor, fontSize);
        context.PopClipRect();

        y += rowH;
    }

    context.PopClipRect();

    // 3. Paint Standard Engine Scrollbar (using ScrollViewport)
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
}

} // namespace we::editor::panels
