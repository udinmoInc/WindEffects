#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

#include "Core/EditorConfigPaths.h"
#include "Core/Logger.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Widgets/DockContainer.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Core/UIRepaintGate.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::programs::editor {
namespace {

constexpr const char* kLayoutFileName = "editor_layout.ini";

constexpr float kMinUsableToolsPaneWidth = 180.0f;
constexpr float kMinUsableContentBrowserHeight = 120.0f;
constexpr float kDefaultToolsPaneWidth = 300.0f;
constexpr float kDefaultContentBrowserHeight = 240.0f;

std::string Trim(const std::string& value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

float SanitizeToolsPaneWidth(float value) {
    if (value < 8.0f) {
        return kDefaultToolsPaneWidth;
    }
    return std::max(value, kMinUsableToolsPaneWidth);
}

float SanitizeContentBrowserHeight(float value) {
    if (value < 8.0f) {
        return kDefaultContentBrowserHeight;
    }
    return std::max(value, kMinUsableContentBrowserHeight);
}

} // namespace

EditorWorkspaceController& EditorWorkspaceController::Get() {
    static EditorWorkspaceController instance;
    return instance;
}

we::runtime::kindui::IPopupHost* GetEditorPopupHost() {
    return EditorWorkspaceController::Get().GetPopupHost();
}

void EditorWorkspaceController::BindLayout(const ::we::editor::shell::DockLayoutBuildResult& layout) {
    m_Layout = layout;
}

void EditorWorkspaceController::SetPopupHost(we::runtime::kindui::OverlayHost* host) {
    m_PopupHost = host;
}

we::runtime::kindui::IPopupHost* EditorWorkspaceController::GetPopupHost() const {
    return m_PopupHost;
}

void EditorWorkspaceController::RegisterPanel(
    const std::string& panelId,
    const std::shared_ptr<::we::editor::panels::Panel>& panel,
    ::we::editor::docking::DockZone zone) {
    if (!panel) {
        return;
    }

    PanelEntry entry;
    entry.panel = panel;
    entry.zone = zone;
    entry.visible = panel->IsVisible();
    m_Panels[panelId] = std::move(entry);
}

std::shared_ptr<::we::editor::docking::DockContainer> EditorWorkspaceController::DockForPanel(
    const std::string& panelId) const {
    if (panelId == "Tools") {
        return m_Layout.toolsDock;
    }
    if (panelId == "Viewport") {
        return m_Layout.viewportDock;
    }
    if (panelId == "WorldOutliner") {
        return m_Layout.explorerDock;
    }
    if (panelId == "Details") {
        return m_Layout.detailsDock;
    }
    if (panelId == "ContentBrowser") {
        return m_Layout.contentBrowserDock;
    }
    return nullptr;
}

std::shared_ptr<::we::editor::docking::DockContainer> EditorWorkspaceController::DockForZone(
    ::we::editor::docking::DockZone zone) const {
    switch (zone) {
    case ::we::editor::docking::DockZone::Left:
        return m_Layout.toolsDock;
    case ::we::editor::docking::DockZone::Center:
        return m_Layout.viewportDock;
    case ::we::editor::docking::DockZone::Right:
        return m_Layout.explorerDock;
    case ::we::editor::docking::DockZone::Bottom:
        return m_Layout.contentBrowserDock;
    default:
        return nullptr;
    }
}

void EditorWorkspaceController::SetPanelVisible(const std::string& panelId, bool visible) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }

    it->second.visible = visible;
    it->second.panel->SetVisible(visible);

    if (auto dock = DockForPanel(panelId)) {
        if (visible) {
            if (!dock->ContainsPanel(it->second.panel)) {
                dock->AddPanel(it->second.panel);
            }
            dock->FocusPanel(it->second.panel);
        } else if (dock->ContainsPanel(it->second.panel)) {
            dock->RemovePanel(it->second.panel);
        }
    } else if (auto zoneDock = DockForZone(it->second.zone)) {
        if (visible) {
            if (!zoneDock->ContainsPanel(it->second.panel)) {
                zoneDock->AddPanel(it->second.panel);
            }
            zoneDock->FocusPanel(it->second.panel);
        } else if (zoneDock->ContainsPanel(it->second.panel)) {
            zoneDock->RemovePanel(it->second.panel);
        }
    }

    if (m_OnPanelVisibilityChanged) {
        m_OnPanelVisibilityChanged();
    }

    we::runtime::kindui::UIRepaintGate::RequestLayout();
}

void EditorWorkspaceController::TogglePanelVisibility(const std::string& panelId) {
    SetPanelVisible(panelId, !IsPanelVisible(panelId));
}

bool EditorWorkspaceController::IsPanelVisible(const std::string& panelId) const {
    const auto it = m_Panels.find(panelId);
    return it != m_Panels.end() ? it->second.visible : false;
}

void EditorWorkspaceController::FloatPanel(const std::string& panelId) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel || !m_PopupHost) {
        return;
    }

    if (auto dock = DockForPanel(panelId)) {
        dock->RemovePanel(it->second.panel);
    } else if (auto zoneDock = DockForZone(it->second.zone)) {
        zoneDock->RemovePanel(it->second.panel);
    }

    m_PopupHost->ShowPopup(it->second.panel, we::runtime::kindui::Point{ 120.0f, 120.0f });
    it->second.visible = true;
    it->second.panel->SetVisible(true);
}

void EditorWorkspaceController::FocusPanel(const std::string& panelId) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }

    if (auto dock = DockForPanel(panelId)) {
        dock->FocusPanel(it->second.panel);
    } else if (auto zoneDock = DockForZone(it->second.zone)) {
        zoneDock->FocusPanel(it->second.panel);
    }
}

void EditorWorkspaceController::ApplyToolsPanelVisibility(bool visible) {
    if (m_Layout.toolsDock) {
        m_Layout.toolsDock->SetVisible(visible);
    }

    if (m_Layout.toolsViewportSplitter) {
        if (visible) {
            m_Layout.toolsViewportSplitter->SetResizeMode(Splitter::ResizeMode::FixedFirst);
            m_Layout.toolsViewportSplitter->SetFixedFirstWidth(
                m_ToolsPaneWidth > 0.0f ? m_ToolsPaneWidth : 300.0f);
        } else {
            const float currentWidth = m_Layout.toolsViewportSplitter->GetFixedFirstWidth();
            if (currentWidth >= kMinUsableToolsPaneWidth) {
                m_ToolsPaneWidth = currentWidth;
            }
            m_Layout.toolsViewportSplitter->SetFixedFirstWidth(0.0f);
        }
    }

    if (auto toolsIt = m_Panels.find("Tools"); toolsIt != m_Panels.end() && toolsIt->second.panel) {
        toolsIt->second.panel->SetVisible(visible);
        toolsIt->second.visible = visible;
    }

    we::runtime::kindui::UIRepaintGate::RequestLayout();
}

void EditorWorkspaceController::SetBottomPanelIndex(int index) {
    if (index == 0) {
        SetPanelVisible("ContentBrowser", true);
        SetPanelVisible("OutputLog", false);
    } else {
        SetPanelVisible("OutputLog", true);
        SetPanelVisible("ContentBrowser", false);
    }
}

void EditorWorkspaceController::FocusViewportNavigationPanel() {
    if (!m_PopupHost) {
        return;
    }

    for (const auto& [panelId, entry] : m_Panels) {
        if (panelId == "ViewportNavigation" && entry.panel) {
            m_PopupHost->CloseAllPopups();
            m_PopupHost->ShowFullscreenPopup(entry.panel);
            return;
        }
    }
}

void EditorWorkspaceController::ToggleContentBrowserExpanded() {
    if (!m_Layout.rootVerticalSplitter) {
        return;
    }

    auto splitter = m_Layout.rootVerticalSplitter;
    m_ContentBrowserExpanded = !m_ContentBrowserExpanded;
    if (m_ContentBrowserExpanded) {
        splitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
        splitter->SetFixedSecondWidth(
            m_ContentBrowserBottomHeight > 0.0f ? m_ContentBrowserBottomHeight : 240.0f);
    } else {
        m_ContentBrowserBottomHeight = splitter->GetFixedSecondWidth();
        splitter->SetFixedSecondWidth(0.0f);
    }

    we::runtime::kindui::UIRepaintGate::RequestLayout();
}

void EditorWorkspaceController::SetOnPanelVisibilityChanged(std::function<void()> callback) {
    m_OnPanelVisibilityChanged = std::move(callback);
}

void EditorWorkspaceController::LoadLayout() {
    const auto path = we::core::ResolveEditorConfigPath(kLayoutFileName);
    if (!std::filesystem::exists(path)) {
        return;
    }

    std::ifstream file(path);
    if (!file) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        const std::string key = Trim(line.substr(0, eq));
        const std::string value = Trim(line.substr(eq + 1));
        const float parsed = std::stof(value);

        if (key == "contentBrowserHeight" && m_Layout.rootVerticalSplitter) {
            if (parsed < 1.0f) {
                m_ContentBrowserExpanded = false;
                m_Layout.rootVerticalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
                m_Layout.rootVerticalSplitter->SetFixedSecondWidth(0.0f);
            } else {
                m_ContentBrowserExpanded = true;
                m_ContentBrowserBottomHeight = SanitizeContentBrowserHeight(parsed);
                m_Layout.rootVerticalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
                m_Layout.rootVerticalSplitter->SetFixedSecondWidth(m_ContentBrowserBottomHeight);
            }
        } else if (key == "toolsPaneWidth" && m_Layout.toolsViewportSplitter) {
            m_ToolsPaneWidth = SanitizeToolsPaneWidth(parsed);
            m_Layout.toolsViewportSplitter->SetResizeMode(Splitter::ResizeMode::FixedFirst);
            m_Layout.toolsViewportSplitter->SetFixedFirstWidth(m_ToolsPaneWidth);
        } else if (key == "mainHorizontalRightWidth" && m_Layout.mainHorizontalSplitter) {
            m_Layout.mainHorizontalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
            m_Layout.mainHorizontalSplitter->SetFixedSecondWidth(std::max(parsed, 200.0f));
        } else if (key == "mainHorizontal" && m_Layout.mainHorizontalSplitter) {
            m_Layout.mainHorizontalSplitter->SetSplitRatio(parsed);
        } else if (key == "rootVertical" && m_Layout.rootVerticalSplitter) {
            if (parsed < 1.0f) {
                m_ContentBrowserExpanded = false;
                m_Layout.rootVerticalSplitter->SetFixedSecondWidth(0.0f);
            } else {
                m_ContentBrowserExpanded = true;
                m_ContentBrowserBottomHeight = SanitizeContentBrowserHeight(parsed);
                m_Layout.rootVerticalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
                m_Layout.rootVerticalSplitter->SetFixedSecondWidth(m_ContentBrowserBottomHeight);
            }
        } else if (key == "toolsViewport" && m_Layout.toolsViewportSplitter) {
            m_ToolsPaneWidth = SanitizeToolsPaneWidth(parsed);
            m_Layout.toolsViewportSplitter->SetResizeMode(Splitter::ResizeMode::FixedFirst);
            m_Layout.toolsViewportSplitter->SetFixedFirstWidth(m_ToolsPaneWidth);
        } else if (key == "contentBrowserStoredHeight" && m_Layout.rootVerticalSplitter) {
            m_ContentBrowserBottomHeight = SanitizeContentBrowserHeight(parsed);
        } else if (key == "rightVertical" && m_Layout.rightVerticalSplitter) {
            m_Layout.rightVerticalSplitter->SetSplitRatio(parsed);
        }
    }
}

void EditorWorkspaceController::SaveLayout() const {
    const auto path = we::core::ResolveEditorConfigPath(kLayoutFileName);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path);
    if (!file) {
        HE_WARN("[EditorWorkspace] Failed to save layout to " + path.string());
        return;
    }

    auto writeFixedSecond = [&](const char* key, const std::shared_ptr<we::runtime::kindui::Splitter>& splitter, float minSize) {
        if (!splitter || splitter->GetResizeMode() != Splitter::ResizeMode::FixedSecond) {
            return;
        }
        const float value = splitter->GetFixedSecondWidth();
        if (value >= minSize) {
            file << key << "=" << value << "\n";
        }
    };
    auto writeFixedFirst = [&](const char* key, float value) {
        if (value >= kMinUsableToolsPaneWidth) {
            file << key << "=" << value << "\n";
        }
    };
    auto writeRatio = [&](const char* key, const std::shared_ptr<we::runtime::kindui::Splitter>& splitter) {
        if (splitter && splitter->GetResizeMode() == Splitter::ResizeMode::Ratio) {
            file << key << "=" << splitter->GetSplitRatio() << "\n";
        }
    };

    if (m_ContentBrowserExpanded) {
        writeFixedSecond("contentBrowserHeight", m_Layout.rootVerticalSplitter, kMinUsableContentBrowserHeight);
    } else {
        file << "contentBrowserHeight=0\n";
        if (m_ContentBrowserBottomHeight >= kMinUsableContentBrowserHeight) {
            file << "contentBrowserStoredHeight=" << m_ContentBrowserBottomHeight << "\n";
        }
    }
    writeFixedFirst("toolsPaneWidth", m_ToolsPaneWidth);
    writeFixedSecond("mainHorizontalRightWidth", m_Layout.mainHorizontalSplitter, 200.0f);
    writeRatio("rightVertical", m_Layout.rightVerticalSplitter);
}

} // namespace we::programs::editor
