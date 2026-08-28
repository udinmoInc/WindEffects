#pragma once

#include "KindUI/Core/Widgets/PanelToolbarRow.h"
#include "KindUI/Core/Types.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include <functional>
#include <string>

namespace we::editor::outliner {

class ExplorerPanelHeader : public we::runtime::kindui::PanelToolbarRow {
public:
    using Rect = we::runtime::kindui::Rect;
    static float DefaultHeight() {
        return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PanelToolbarHeight);
    }

    using FilterOptions = ::we::editor::contentbrowser::TreeView::FilterOptions;

    ExplorerPanelHeader();

    void Initialize();

    Rect GetFilterButtonGeometry() const;

    void SetSearchQuery(const std::string& query);
    std::string GetSearchQuery() const;
    void SetOnSearchChanged(std::function<void(const std::string&)> callback);
    void SetOnFilterClicked(std::function<void()> callback);
    void SetOnNewFolder(std::function<void()> callback);
    void SetOnRefresh(std::function<void()> callback);

    FilterOptions GetFilterOptions() const { return m_FilterOptions; }
    void SetFilterOptions(const FilterOptions& options) { m_FilterOptions = options; }

protected:
    void EnsureBuilt();

private:
    FilterOptions m_FilterOptions;
    std::function<void()> m_OnFilterClicked;
    std::function<void()> m_OnNewFolder;
    std::function<void()> m_OnRefresh;
};

} // namespace we::editor::outliner
