#include "Platform/Platform.h"
#include "Widgets/ExplorerPanelHeader.h"

using namespace we::runtime::kindui;

namespace we::editor::outliner {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

ExplorerPanelHeader::ExplorerPanelHeader()
    : PanelToolbarRow("Search...") {}

void ExplorerPanelHeader::Initialize() {
    AddIconButton(kWindIconNone, [this]() {
        if (m_OnNewFolder) {
            m_OnNewFolder();
        }
    });
    AddIconButton(kWindIconNone, [this]() {
        if (m_OnFilterClicked) {
            m_OnFilterClicked();
        }
    });

    Finalize();
}

Rect ExplorerPanelHeader::GetFilterButtonGeometry() const {
    if (auto btn = GetIconButton(1)) {
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
