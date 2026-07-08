#include "Widgets/OutputLogWidget.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"

namespace we::UI {

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

void OutputLogWidget::Clear() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    m_Records.clear();
    m_VisibleLines.clear();
    m_VisibleLevels.clear();
    m_ScrollOffset = 0.0f;
}

void OutputLogWidget::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    RebuildVisibleLines();
}

Color OutputLogWidget::LevelColor(we::Logger::Level level) const {
    const auto& theme = Theme::Get();
    switch (level) {
        case we::Logger::Level::Trace: return theme.TextMuted;
        case we::Logger::Level::Debug: return Color{ 0.65f, 0.75f, 0.85f, 1.0f };
        case we::Logger::Level::Info: return theme.TextSecondary;
        case we::Logger::Level::Warning: return theme.Warning;
        case we::Logger::Level::Error: return theme.AccentPrimary;
        case we::Logger::Level::Critical: return Color{ 1.0f, 0.15f, 0.15f, 1.0f };
    }
    return theme.TextPrimary;
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
    for (const auto& record : m_Records) {
        if (!PassesFilter(record)) continue;
        m_VisibleLines.push_back(record.formattedText);
        m_VisibleLevels.push_back(record.level);
    }
    if (m_AutoScroll) {
        const float contentHeight = static_cast<float>(m_VisibleLines.size()) * 14.0f;
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

    const Theme& theme = Theme::Get();
    context.DrawRect(geometry, theme.PanelBackground);

    const float lineHeight = 14.0f;
    float y = geometry.y + 4.0f - scrollOffset;
    const float maxY = geometry.y + geometry.height;

    for (size_t i = 0; i < visibleLines.size(); ++i) {
        if (y + lineHeight < geometry.y) {
            y += lineHeight;
            continue;
        }
        if (y > maxY) break;
        context.DrawText(
            visibleLines[i],
            Point{ geometry.x + 6.0f, y },
            LevelColor(visibleLevels[i]),
            12.0f);
        y += lineHeight;
    }
}

} // namespace we::UI
