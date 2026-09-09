#include "Platform/Platform.h"
#include "Widgets/ExplorerPanelHeader.h"

using namespace we::runtime::kindui;

namespace we::editor::outliner {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

ExplorerPanelHeader::ExplorerPanelHeader()
    : PanelToolbarRow("Search...") {}

void ExplorerPanelHeader::Initialize() {
    AddLeadingIconButton(WindIcons::ListFilter16, [this]() {
        if (m_OnFilterClicked) {
            m_OnFilterClicked();
        }
    });
    AddIconButton(WindIcons::FolderCreate16, [this]() {
        if (m_OnNewFolder) {
            m_OnNewFolder();
        }
    });
    AddIconButton(WindIcons::Settings16, [this]() {
        // Normal Settings
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
