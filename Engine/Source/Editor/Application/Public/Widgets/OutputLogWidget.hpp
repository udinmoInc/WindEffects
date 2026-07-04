#pragma once

#include "Core/Widget.hpp"
#include "Core/Logger.hpp"
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace we::UI {

class OutputLogWidget : public Widget {
public:
    OutputLogWidget();
    ~OutputLogWidget() override;

    void Tick(float deltaTime) override;
    void Paint(PaintContext& context) override;

    void SetPaused(bool paused) { m_Paused = paused; }
    bool IsPaused() const { return m_Paused; }
    void SetAutoScroll(bool enabled) { m_AutoScroll = enabled; }
    void Clear();
    void SetSearchQuery(const std::string& query);
    void SetMinimumLevel(we::Logger::Level level) { m_MinLevel = level; }
    void SetCategoryFilter(const std::string& category) { m_CategoryFilter = category; }

private:
    we::UI::Color LevelColor(we::Logger::Level level) const;
    bool PassesFilter(const we::Logger::LogRecord& record) const;
    void RebuildVisibleLines();

    std::deque<we::Logger::LogRecord> m_Records;
    std::vector<std::string> m_VisibleLines;
    std::vector<we::Logger::Level> m_VisibleLevels;
    std::mutex m_Mutex;
    std::string m_SearchQuery;
    std::string m_CategoryFilter;
    we::Logger::Level m_MinLevel = we::Logger::Level::Trace;
    bool m_Paused = false;
    bool m_AutoScroll = true;
    float m_ScrollOffset = 0.0f;
    static constexpr size_t kMaxStoredRecords = 5000;
};

} // namespace we::UI
