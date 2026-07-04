#include "Widgets/OutputLogWidget.hpp"
#include "Core/Theme.hpp"

namespace we::UI {

OutputLogWidget::OutputLogWidget() {
    for (const auto& record : we::Logger::GetHistory()) {
        m_Records.push_back(record);
    }
    RebuildVisibleLines();

    we::Logger::AddListener([this](const we::Logger::LogRecord& record) {
        if (m_Paused) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Records.push_back(record);
        while (m_Records.size() > kMaxStoredRecords) {
            m_Records.pop_front();
        }
    });
}

OutputLogWidget::~OutputLogWidget() = default;

void OutputLogWidget::Tick(float /*deltaTime*/) {
    if (m_Paused) return;

    const auto newLogs = we::Logger::GetNewLogs();
    if (newLogs.empty()) return;

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
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
    std::lock_guard<std::mutex> lock(m_Mutex);
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
    switch (level) {
        case we::Logger::Level::Trace: return Color{ 0.55f, 0.55f, 0.55f, 1.0f };
        case we::Logger::Level::Debug: return Color{ 0.65f, 0.75f, 0.85f, 1.0f };
        case we::Logger::Level::Info: return Color{ 0.85f, 0.85f, 0.85f, 1.0f };
        case we::Logger::Level::Warning: return Color{ 0.95f, 0.78f, 0.25f, 1.0f };
        case we::Logger::Level::Error: return Color{ 0.95f, 0.35f, 0.30f, 1.0f };
        case we::Logger::Level::Critical: return Color{ 1.0f, 0.15f, 0.15f, 1.0f };
    }
    return Color::White();
}

bool OutputLogWidget::PassesFilter(const we::Logger::LogRecord& record) const {
    if (static_cast<int>(record.level) < static_cast<int>(m_MinLevel)) return false;
    if (!m_CategoryFilter.empty() && record.category != m_CategoryFilter) return false;
    if (!m_SearchQuery.empty() && record.formattedText.find(m_SearchQuery) == std::string::npos) return false;
    return true;
}

void OutputLogWidget::RebuildVisibleLines() {
    std::lock_guard<std::mutex> lock(m_Mutex);
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

    const Theme& theme = Theme::Get();
    context.DrawRect(m_Geometry, theme.PanelBackground);

    const float lineHeight = 14.0f;
    float y = m_Geometry.y + 4.0f - m_ScrollOffset;
    const float maxY = m_Geometry.y + m_Geometry.height;

    for (size_t i = 0; i < m_VisibleLines.size(); ++i) {
        if (y + lineHeight < m_Geometry.y) {
            y += lineHeight;
            continue;
        }
        if (y > maxY) break;
        context.DrawText(
            m_VisibleLines[i],
            Point{ m_Geometry.x + 6.0f, y },
            LevelColor(m_VisibleLevels[i]),
            12.0f);
        y += lineHeight;
    }
}

} // namespace we::UI
