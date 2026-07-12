#include "WindEffects/Editor/UI/Docking/DockManager.h"

#include <fstream>
#include <sstream>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace WindEffects::Editor::UI {
namespace {

#if WE_HAS_NLOHMANN_JSON

const char* ToString(DockNodeType type) {
    switch (type) {
    case DockNodeType::Split: return "Split";
    case DockNodeType::TabGroup: return "TabGroup";
    case DockNodeType::Panel: return "Panel";
    case DockNodeType::Root: return "Root";
    }
    return "Root";
}

const char* ToString(SplitOrientation orientation) {
    return orientation == SplitOrientation::Vertical ? "Vertical" : "Horizontal";
}

const char* ToString(DockZone zone) {
    switch (zone) {
    case DockZone::Left: return "Left";
    case DockZone::Center: return "Center";
    case DockZone::Right: return "Right";
    case DockZone::Bottom: return "Bottom";
    case DockZone::Floating: return "Floating";
    }
    return "Floating";
}

bool TryParseNodeType(std::string_view value, DockNodeType& out) {
    if (value == "Split") { out = DockNodeType::Split; return true; }
    if (value == "TabGroup") { out = DockNodeType::TabGroup; return true; }
    if (value == "Panel") { out = DockNodeType::Panel; return true; }
    if (value == "Root") { out = DockNodeType::Root; return true; }
    return false;
}

bool TryParseOrientation(std::string_view value, SplitOrientation& out) {
    if (value == "Vertical") { out = SplitOrientation::Vertical; return true; }
    if (value == "Horizontal") { out = SplitOrientation::Horizontal; return true; }
    return false;
}

bool TryParseDockZone(std::string_view value, DockZone& out) {
    if (value == "Left") { out = DockZone::Left; return true; }
    if (value == "Center") { out = DockZone::Center; return true; }
    if (value == "Right") { out = DockZone::Right; return true; }
    if (value == "Bottom") { out = DockZone::Bottom; return true; }
    if (value == "Floating") { out = DockZone::Floating; return true; }
    return false;
}

nlohmann::json SerializeNode(const DockLayoutNode& node) {
    nlohmann::json json;
    json["type"] = ToString(node.type);
    json["orientation"] = ToString(node.orientation);
    json["splitRatio"] = node.splitRatio;
    if (!node.panelId.empty()) {
        json["panelId"] = node.panelId;
    }
    if (!node.children.empty()) {
        json["children"] = nlohmann::json::array();
        for (const auto& child : node.children) {
            json["children"].push_back(SerializeNode(child));
        }
    }
    return json;
}

bool DeserializeNode(const nlohmann::json& json, DockLayoutNode& node) {
    if (!json.is_object()) {
        return false;
    }

    if (json.contains("type")) {
        DockNodeType type = DockNodeType::Root;
        if (!TryParseNodeType(json["type"].get<std::string>(), type)) {
            return false;
        }
        node.type = type;
    }

    if (json.contains("orientation")) {
        SplitOrientation orientation = SplitOrientation::Horizontal;
        if (!TryParseOrientation(json["orientation"].get<std::string>(), orientation)) {
            return false;
        }
        node.orientation = orientation;
    }

    if (json.contains("splitRatio")) {
        node.splitRatio = json["splitRatio"].get<float>();
    }

    if (json.contains("panelId")) {
        node.panelId = json["panelId"].get<std::string>();
    }

    node.children.clear();
    if (json.contains("children") && json["children"].is_array()) {
        for (const auto& childJson : json["children"]) {
            DockLayoutNode child;
            if (!DeserializeNode(childJson, child)) {
                return false;
            }
            node.children.push_back(std::move(child));
        }
    }

    return true;
}

nlohmann::json SerializePanelDescriptor(const DockPanelDescriptor& panel) {
    nlohmann::json json;
    json["id"] = panel.id;
    json["title"] = panel.title;
    json["iconResource"] = panel.iconResource;
    json["defaultZone"] = ToString(panel.defaultZone);
    json["closable"] = panel.closable;
    json["floatable"] = panel.floatable;
    json["defaultVisible"] = panel.defaultVisible;
    json["sortOrder"] = panel.sortOrder;
    return json;
}

bool DeserializePanelDescriptor(const nlohmann::json& json, DockPanelDescriptor& panel) {
    if (!json.is_object()) {
        return false;
    }

    if (json.contains("id")) panel.id = json["id"].get<std::string>();
    if (json.contains("title")) panel.title = json["title"].get<std::string>();
    if (json.contains("iconResource")) panel.iconResource = json["iconResource"].get<std::string>();
    if (json.contains("defaultZone")) {
        DockZone zone = DockZone::Floating;
        if (!TryParseDockZone(json["defaultZone"].get<std::string>(), zone)) {
            return false;
        }
        panel.defaultZone = zone;
    }
    if (json.contains("closable")) panel.closable = json["closable"].get<bool>();
    if (json.contains("floatable")) panel.floatable = json["floatable"].get<bool>();
    if (json.contains("defaultVisible")) panel.defaultVisible = json["defaultVisible"].get<bool>();
    if (json.contains("sortOrder")) panel.sortOrder = json["sortOrder"].get<int>();
    return true;
}

#endif // WE_HAS_NLOHMANN_JSON

} // namespace

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
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json json;
    json["workspaceId"] = layout.workspaceId;
    json["root"] = SerializeNode(layout.root);

    json["panels"] = nlohmann::json::object();
    for (const auto& [id, panel] : layout.panels) {
        (void)id;
        json["panels"][panel.id] = SerializePanelDescriptor(panel);
    }

    std::ofstream file{std::string(path)};
    if (!file) {
        return false;
    }
    file << json.dump(2);
    return static_cast<bool>(file);
#else
    std::ofstream file{std::string(path)};
    if (!file) return false;
    file << "{\n";
    file << "  \"workspaceId\": \"" << layout.workspaceId << "\"\n";
    file << "}\n";
    return true;
#endif
}

bool DockLayoutSerializer::Load(WorkspaceLayout& layout, std::string_view path) const {
#if WE_HAS_NLOHMANN_JSON
    std::ifstream file{std::string(path)};
    if (!file) {
        return false;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception&) {
        return false;
    }

    if (!json.is_object()) {
        return false;
    }

    WorkspaceLayout loaded;
    if (json.contains("workspaceId")) {
        loaded.workspaceId = json["workspaceId"].get<std::string>();
    }

    if (json.contains("root")) {
        if (!DeserializeNode(json["root"], loaded.root)) {
            return false;
        }
    }

    if (json.contains("panels") && json["panels"].is_object()) {
        for (const auto& [id, panelJson] : json["panels"].items()) {
            DockPanelDescriptor panel;
            if (!DeserializePanelDescriptor(panelJson, panel)) {
                return false;
            }
            if (panel.id.empty()) {
                panel.id = id;
            }
            loaded.panels[panel.id] = std::move(panel);
        }
    }

    layout = std::move(loaded);
    return true;
#else
    (void)layout;
    (void)path;
    return false;
#endif
}

WorkspaceLayout CreateDefaultEditorWorkspaceLayout() {
    WorkspaceLayout layout;
    layout.workspaceId = "Default";

    layout.panels["Tools"] = {
        .id = "Tools",
        .title = "Actors",
        .iconResource = "tools-panel",
        .defaultZone = DockZone::Left,
        .defaultVisible = true,
        .sortOrder = 0
    };
    layout.panels["Viewport"] = {
        .id = "Viewport",
        .title = "Viewport",
        .iconResource = "viewport",
        .defaultZone = DockZone::Center,
        .defaultVisible = true,
        .sortOrder = 1
    };
    layout.panels["WorldOutliner"] = {
        .id = "WorldOutliner",
        .title = "Explorer",
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
            {.type = DockNodeType::TabGroup, .panelId = "Details"}
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
            {.type = DockNodeType::TabGroup, .panelId = "ContentBrowser"}
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
