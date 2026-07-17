#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "KindUI/Core/Widget.h"
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

class UIFRAMEWORK_API OutputLogWidget : public Widget {
public:
    OutputLogWidget();
    ~OutputLogWidget() override;

    void Tick(float deltaTime) override;
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetPaused(bool paused) { m_Paused = paused; }
    bool IsPaused() const { return m_Paused; }
    void SetAutoScroll(bool enabled) { m_AutoScroll = enabled; }
    void Clear();
    void SetSearchQuery(const std::string& query);
    void SetMinimumLevel(we::Logger::Level level) { m_MinLevel = level; }
    void SetCategoryFilter(const std::string& category) { m_CategoryFilter = category; }

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
    static constexpr size_t kMaxStoredRecords = 5000;
};

} // namespace we::editor::panels
