#include "EditorModeController.h"
#include "EditorLayoutController.h"
#include "EditorToolsRegistry.h"
#include "Core/Logger.h"
#include "Core/EditorConfigPaths.h"

#include <filesystem>
#include <fstream>
#include <algorithm>

namespace we::programs::editor {

namespace {

std::filesystem::path GetStatePath() {
    return we::core::ResolveEditorConfigPath("editor_mode.ini");
}

} // namespace

EditorModeController& EditorModeController::Get() {
    static EditorModeController instance;
    return instance;
}

void EditorModeController::InitializeFromRegistry() {
    const bool hadSavedState = std::filesystem::exists(GetStatePath());
    LoadState();

    if (!EditorToolsRegistry::Get().FindMode(m_ActiveModeId)) {
        auto modes = EditorToolsRegistry::Get().GetModesSorted();
        if (!modes.empty()) {
            m_ActiveModeId = modes.front()->id;
        }
    }

    if (!hadSavedState) {
        m_ActiveModeId = "Actors";
        m_DrawerVisible = true;
    }

    HE_INFO("[EditorMode] Active mode: " + m_ActiveModeId
        + " drawer=" + (m_DrawerVisible ? "visible" : "hidden"));
}

void EditorModeController::SetActiveMode(const std::string& modeId) {
    if (!EditorToolsRegistry::Get().FindMode(modeId)) {
        HE_ERROR("[EditorMode] Unknown mode: " + modeId);
        return;
    }
    if (m_ActiveModeId == modeId) {
        if (!m_DrawerVisible) {
            SetDrawerVisible(true);
        }
        return;
    }

    m_ActiveModeId = modeId;
    if (!m_DrawerPinned) {
        ApplyModeDrawerPolicy();
    }
    SaveState();
    NotifyModeChanged();
}

void EditorModeController::SetDrawerVisible(bool visible) {
    if (m_DrawerVisible == visible) return;
    m_DrawerVisible = visible;
    EditorLayoutController::Get().ApplyToolsPanelVisibility(visible);
    SaveState();
    NotifyModeChanged();
}

void EditorModeController::SetDrawerCollapsed(bool collapsed) {
    if (m_DrawerCollapsed == collapsed) return;
    m_DrawerCollapsed = collapsed;
    SaveState();
    NotifyModeChanged();
}

void EditorModeController::SetDrawerPinned(bool pinned) {
    if (m_DrawerPinned == pinned) return;
    m_DrawerPinned = pinned;
    SaveState();
    NotifyModeChanged();
}

void EditorModeController::SetDrawerWidth(float width) {
    m_DrawerWidth = std::clamp(width, 260.0f, 300.0f);
    SaveState();
    NotifyModeChanged();
}

void EditorModeController::ToggleDrawer() {
    if (m_DrawerVisible) {
        SetDrawerVisible(false);
        return;
    }

    if (m_ActiveModeId == "Select") {
        SetActiveMode("Actors");
        return;
    }

    SetDrawerVisible(true);
}

void EditorModeController::AddModeChangedListener(ModeChangedCallback callback) {
    m_ModeListeners.push_back(std::move(callback));
}

void EditorModeController::ApplyModeDrawerPolicy() {
    const auto* mode = EditorToolsRegistry::Get().FindMode(m_ActiveModeId);
    if (!mode) {
        m_DrawerVisible = false;
        return;
    }
    m_DrawerVisible = mode->opensToolDrawerByDefault;
}

void EditorModeController::NotifyModeChanged() {
    for (const auto& listener : m_ModeListeners) {
        if (listener) listener(m_ActiveModeId);
    }
}

void EditorModeController::LoadState() {
    const auto path = GetStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "activeModeId") m_ActiveModeId = value;
        else if (key == "drawerVisible") m_DrawerVisible = (value == "1");
        else if (key == "drawerCollapsed") m_DrawerCollapsed = (value == "1");
        else if (key == "drawerPinned") m_DrawerPinned = (value == "1");
        else if (key == "drawerWidth") m_DrawerWidth = std::stof(value);
    }
}

void EditorModeController::SaveState() const {
    const auto path = GetStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return;

    file << "activeModeId=" << m_ActiveModeId << "\n";
    file << "drawerVisible=" << (m_DrawerVisible ? "1" : "0") << "\n";
    file << "drawerCollapsed=" << (m_DrawerCollapsed ? "1" : "0") << "\n";
    file << "drawerPinned=" << (m_DrawerPinned ? "1" : "0") << "\n";
    file << "drawerWidth=" << m_DrawerWidth << "\n";
}

} // namespace we::programs::editor
