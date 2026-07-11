#include "WindEffects/Editor/UI/Docking/DockManager.h"

#include <fstream>

namespace WindEffects::Editor::UI {

void DockManager::RegisterPanel(const DockPanelDescriptor& descriptor, PanelFactory factory) {
    std::lock_guard lock(m_Mutex);
    PanelEntry entry;
    entry.descriptor = descriptor;
    entry.factory = std::move(factory);
    entry.visible = descriptor.defaultVisible;
    entry.currentZone = descriptor.defaultZone;
    m_Panels[descriptor.id] = std::move(entry);
    m_Layout.panels[descriptor.id] = descriptor;
}

void DockManager::UnregisterPanel(std::string_view panelId) {
    std::lock_guard lock(m_Mutex);
    m_Panels.erase(std::string(panelId));
    m_Layout.panels.erase(std::string(panelId));
}

void DockManager::SetLayout(WorkspaceLayout layout) {
    std::lock_guard lock(m_Mutex);
    m_Layout = std::move(layout);
}

std::shared_ptr<void> DockManager::GetPanelWidget(std::string_view panelId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Panels.find(std::string(panelId));
    if (it == m_Panels.end()) {
        return nullptr;
    }

    if (!it->second.instance && it->second.factory) {
        it->second.instance = it->second.factory();
    }
    return it->second.instance;
}

void DockManager::SetPanelVisible(std::string_view panelId, bool visible) {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Panels.find(std::string(panelId));
    if (it != m_Panels.end()) {
        it->second.visible = visible;
    }
}

bool DockManager::IsPanelVisible(std::string_view panelId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Panels.find(std::string(panelId));
    return it != m_Panels.end() ? it->second.visible : false;
}

void DockManager::FloatPanel(std::string_view panelId) {
    DockPanel(panelId, DockZone::Floating);
}

void DockManager::DockPanel(std::string_view panelId, DockZone zone) {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Panels.find(std::string(panelId));
    if (it != m_Panels.end()) {
        it->second.currentZone = zone;
        it->second.visible = true;
    }
}

bool DockLayoutSerializer::Save(const WorkspaceLayout& layout, std::string_view path) const {
    std::ofstream file{std::string(path)};
    if (!file) return false;
    file << "{\n";
    file << "  \"workspaceId\": \"" << layout.workspaceId << "\",\n";
    file << "  \"splitRatios\": {\n";
    file << "    \"mainHorizontal\": 0.78,\n";
    file << "    \"leftCenterVertical\": 0.70,\n";
    file << "    \"toolsViewportHorizontal\": 0.18,\n";
    file << "    \"rightVertical\": 0.40\n";
    file << "  }\n";
    file << "}\n";
    return true;
}

bool DockLayoutSerializer::Load(WorkspaceLayout& layout, std::string_view path) const {
    (void)layout;
    (void)path;
    return false;
}

WorkspaceLayout CreateDefaultEditorWorkspaceLayout() {
    WorkspaceLayout layout;
    layout.workspaceId = "Default";

    layout.panels["Tools"] = {
        .id = "Tools",
        .title = "Place Actors",
        .iconResource = "tools-panel",
        .defaultZone = DockZone::Left,
        .defaultVisible = true,
        .sortOrder = 0
    };
    layout.panels["Viewport"] = {
        .id = "Viewport",
        .title = "Viewport 1",
        .iconResource = "viewport",
        .defaultZone = DockZone::Center,
        .defaultVisible = true,
        .sortOrder = 1
    };
    layout.panels["WorldOutliner"] = {
        .id = "WorldOutliner",
        .title = "Environment",
        .iconResource = "outliner",
        .defaultZone = DockZone::Right,
        .defaultVisible = true,
        .sortOrder = 2
    };
    layout.panels["Details"] = {
        .id = "Details",
        .title = "Details",
        .iconResource = "details",
        .defaultZone = DockZone::Right,
        .defaultVisible = true,
        .sortOrder = 3
    };
    layout.panels["ContentBrowser"] = {
        .id = "ContentBrowser",
        .title = "Content Browser",
        .iconResource = "content-browser",
        .defaultZone = DockZone::Bottom,
        .defaultVisible = true,
        .sortOrder = 4
    };
    layout.panels["OutputLog"] = {
        .id = "OutputLog",
        .title = "Output Log",
        .iconResource = "output-log",
        .defaultZone = DockZone::Floating,
        .defaultVisible = false,
        .sortOrder = 5
    };

    // Root split: main body (78%) | right sidebar (22%)
    DockLayoutNode rightSidebar{
        .type = DockNodeType::Split,
        .orientation = SplitOrientation::Vertical,
        .splitRatio = 0.40f,
        .children = {
            {.type = DockNodeType::TabGroup, .panelId = "WorldOutliner"},
            {.type = DockNodeType::Panel, .panelId = "Details"}
        }
    };

    DockLayoutNode toolsViewport{
        .type = DockNodeType::Split,
        .orientation = SplitOrientation::Horizontal,
        .splitRatio = 0.18f,
        .children = {
            {.type = DockNodeType::TabGroup, .panelId = "Tools"},
            {.type = DockNodeType::TabGroup, .panelId = "Viewport"}
        }
    };

    DockLayoutNode leftCenter{
        .type = DockNodeType::Split,
        .orientation = SplitOrientation::Vertical,
        .splitRatio = 0.70f,
        .children = {
            toolsViewport,
            {.type = DockNodeType::Panel, .panelId = "ContentBrowser"}
        }
    };

    layout.root = {
        .type = DockNodeType::Split,
        .orientation = SplitOrientation::Horizontal,
        .splitRatio = 0.78f,
        .children = {leftCenter, rightSidebar}
    };

    return layout;
}

} // namespace WindEffects::Editor::UI
