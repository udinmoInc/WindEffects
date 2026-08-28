#include "Platform/Platform.h"
#include "Widgets/ExplorerPanelHeader.h"

using namespace we::runtime::kindui;

namespace we::editor::outliner {
namespace Icons = ::we::runtime::kindui::Icons;

ExplorerPanelHeader::ExplorerPanelHeader()
    : PanelToolbarRow("Search Actors...") {}

void ExplorerPanelHeader::Initialize() {
    AddIconButton(Icons::FilterName, [this]() {
        if (m_OnFilterClicked) {
            m_OnFilterClicked();
        }
    });
    AddIconButton(Icons::PlusName, [this]() {
        if (m_OnNewFolder) {
            m_OnNewFolder();
        }
    });
    AddIconButton(Icons::RefreshName, [this]() {
        if (m_OnRefresh) {
            m_OnRefresh();
        }
    });

    Finalize();
}

Rect ExplorerPanelHeader::GetFilterButtonGeometry() const {
    if (auto btn = GetIconButton(0)) {
        return btn->GetGeometry();
    }
    return {};
}

void ExplorerPanelHeader::SetSearchQuery(const std::string& query) {
    SetSearchText(query);
}

std::string ExplorerPanelHeader::GetSearchQuery() const {
    return GetSearchText();
}

void ExplorerPanelHeader::SetOnSearchChanged(std::function<void(const std::string&)> callback) {
    PanelToolbarRow::SetOnSearchChanged(std::move(callback));
}

void ExplorerPanelHeader::SetOnFilterClicked(std::function<void()> callback) {
    m_OnFilterClicked = std::move(callback);
}

void ExplorerPanelHeader::SetOnNewFolder(std::function<void()> callback) {
    m_OnNewFolder = std::move(callback);
}

void ExplorerPanelHeader::SetOnRefresh(std::function<void()> callback) {
    m_OnRefresh = std::move(callback);
}

} // namespace we::editor::outliner
