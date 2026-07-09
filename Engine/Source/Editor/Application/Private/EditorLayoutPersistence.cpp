#include "EditorLayoutPersistence.h"

#include "EditorPanelController.h"
#include "Layout/Splitter.h"
#include "Widgets/DockContainer.h"
#include "Core/EditorConfigPaths.h"
#include "Core/Logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace we::programs::editor {

namespace {

constexpr int kLayoutVersion = 3;
constexpr float kMinSplitRatio = 0.05f;
constexpr float kMaxSplitRatio = 0.95f;

float ParseClampedRatio(const std::unordered_map<std::string, std::string>& values,
                        const char* key,
                        float fallback) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }

    try {
        const float v = std::stof(it->second);
        if (!std::isfinite(v)) {
            return fallback;
        }
        return std::clamp(v, kMinSplitRatio, kMaxSplitRatio);
    } catch (...) {
        return fallback;
    }
}

std::filesystem::path GetLayoutPath() {
    return we::core::ResolveEditorConfigPath("editor_layout.ini");
}

std::string PanelKey(EditorPanelId id) {
    switch (id) {
    case EditorPanelId::Viewport: return "Viewport";
    case EditorPanelId::Game: return "Game";
    case EditorPanelId::Tools: return "Tools";
    case EditorPanelId::ContentBrowser: return "ContentBrowser";
    case EditorPanelId::Explorer: return "Explorer";
    case EditorPanelId::Details: return "Details";
    case EditorPanelId::ViewportNavigation: return "ViewportNavigation";
    case EditorPanelId::Debug: return "Debug";
    }
    return "Unknown";
}

} // namespace

EditorLayoutPersistence& EditorLayoutPersistence::Get() {
    static EditorLayoutPersistence instance;
    return instance;
}

void EditorLayoutPersistence::BindLayout(
    const std::shared_ptr<we::UI::Splitter>& mainHorizontal,
    const std::shared_ptr<we::UI::Splitter>& leftCenterVertical,
    const std::shared_ptr<we::UI::Splitter>& editorTopRow,
    const std::shared_ptr<we::UI::Splitter>& rightSideVertical,
    const std::shared_ptr<we::UI::DockContainer>& explorerDock,
    const std::shared_ptr<we::UI::DockContainer>& centerDock) {
    m_MainHorizontal = mainHorizontal;
    m_LeftCenterVertical = leftCenterVertical;
    m_EditorTopRow = editorTopRow;
    m_RightSideVertical = rightSideVertical;
    m_ExplorerDock = explorerDock;
    m_CenterDock = centerDock;
}

void EditorLayoutPersistence::Load() {
    const auto path = GetLayoutPath();
    if (!std::filesystem::exists(path)) {
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        values[line.substr(0, eq)] = line.substr(eq + 1);
    }

    if (auto splitter = m_MainHorizontal.lock(); splitter && values.count("main_h_ratio")) {
        bool allowLoad = true;
        if (values.count("layout_version")) {
            try {
                allowLoad = std::stoi(values.at("layout_version")) >= kLayoutVersion;
            } catch (...) {
                allowLoad = true;
            }
        }
        if (allowLoad) {
            splitter->SetSplitRatio(ParseClampedRatio(values, "main_h_ratio", splitter->GetSplitRatio()));
        }
    }
    if (auto splitter = m_LeftCenterVertical.lock(); splitter && values.count("left_center_v_ratio")) {
        splitter->SetSplitRatio(ParseClampedRatio(values, "left_center_v_ratio", splitter->GetSplitRatio()));
    }
    if (auto splitter = m_EditorTopRow.lock(); splitter && values.count("editor_top_h_ratio")) {
        splitter->SetSplitRatio(ParseClampedRatio(values, "editor_top_h_ratio", splitter->GetSplitRatio()));
    }
    if (auto splitter = m_RightSideVertical.lock(); splitter && values.count("right_side_v_ratio")) {
        splitter->SetSplitRatio(ParseClampedRatio(values, "right_side_v_ratio", splitter->GetSplitRatio()));
    }
    if (auto dock = m_ExplorerDock.lock(); dock && values.count("explorer_dock_tab")) {
        try {
            dock->SetActiveTab(std::stoi(values["explorer_dock_tab"]));
        } catch (...) {
            // ignore invalid persisted value
        }
    }
    if (auto dock = m_CenterDock.lock()) {
        const int tabCount = dock->GetTabCount();
        if (tabCount > 0) {
            if (values.count("center_dock_tab")) {
                try {
                    int tab = std::stoi(values["center_dock_tab"]);
                    tab = std::max(0, std::min(tab, tabCount - 1));
                    dock->SetActiveTab(tab);
                } catch (...) {
                    dock->SetActiveTab(0);
                }
            } else {
                dock->SetActiveTab(0);
            }
        }
    }

    auto& panels = EditorPanelController::Get();
    for (const char* key : { "Explorer", "Details", "ViewportNavigation", "ContentBrowser", "Viewport", "Game", "Debug" }) {
        const std::string visKey = std::string("visible_") + key;
        if (!values.count(visKey)) {
            continue;
        }
        const bool visible = values[visKey] == "1";
        EditorPanelId id = EditorPanelId::Explorer;
        if (std::string(key) == "Details") id = EditorPanelId::Details;
        else if (std::string(key) == "ViewportNavigation") id = EditorPanelId::ViewportNavigation;
        else if (std::string(key) == "ContentBrowser") id = EditorPanelId::ContentBrowser;
        else if (std::string(key) == "Viewport") id = EditorPanelId::Viewport;
        else if (std::string(key) == "Game") id = EditorPanelId::Game;
        else if (std::string(key) == "Debug") id = EditorPanelId::Debug;
        panels.SetPanelVisible(id, visible);
    }

    HE_INFO("[EditorLayout] Loaded layout from " + path.string());
}

void EditorLayoutPersistence::Save() const {
    const auto path = GetLayoutPath();
    std::filesystem::create_directories(path.parent_path());

    std::ostringstream out;
    out << "layout_version=" << kLayoutVersion << '\n';
    if (auto splitter = m_MainHorizontal.lock()) {
        out << "main_h_ratio=" << splitter->GetSplitRatio() << '\n';
    }
    if (auto splitter = m_LeftCenterVertical.lock()) {
        out << "left_center_v_ratio=" << splitter->GetSplitRatio() << '\n';
    }
    if (auto splitter = m_EditorTopRow.lock()) {
        out << "editor_top_h_ratio=" << splitter->GetSplitRatio() << '\n';
    }
    if (auto splitter = m_RightSideVertical.lock()) {
        out << "right_side_v_ratio=" << splitter->GetSplitRatio() << '\n';
    }
    if (auto dock = m_ExplorerDock.lock()) {
        out << "explorer_dock_tab=" << dock->GetActiveTab() << '\n';
    }
    if (auto dock = m_CenterDock.lock()) {
        out << "center_dock_tab=" << dock->GetActiveTab() << '\n';
    }

    const auto& panels = EditorPanelController::Get();
    for (EditorPanelId id : {
             EditorPanelId::Explorer,
             EditorPanelId::Details,
             EditorPanelId::ViewportNavigation,
             EditorPanelId::ContentBrowser,
             EditorPanelId::Viewport,
             EditorPanelId::Game,
             EditorPanelId::Debug }) {
        out << "visible_" << PanelKey(id) << '=' << (panels.IsPanelVisible(id) ? "1" : "0") << '\n';
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        HE_ERROR("[EditorLayout] Failed to save layout to " + path.string());
        return;
    }
    file << out.str();
}

} // namespace we::programs::editor
