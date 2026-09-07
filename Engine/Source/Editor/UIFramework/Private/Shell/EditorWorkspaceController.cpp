#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

#include "Core/EditorConfigPaths.h"
#include "Core/Logger.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Widgets/DockContainer.h"
#include "WindEffects/Editor/UI/Widgets/FloatingPanelFrame.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Widgets/DropdownMenu.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

namespace we::programs::editor {
namespace {

using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;

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
    ApplyToolsPaneWidth(m_ToolsPaneWidth);
}

void EditorWorkspaceController::ApplyToolsPaneWidth(float width) {
    const float paneWidth = SanitizeToolsPaneWidth(width);
    m_ToolsPaneWidth = paneWidth;

    if (m_Layout.toolsViewportSplitter) {
        m_Layout.toolsViewportSplitter->SetResizeMode(Splitter::ResizeMode::FixedFirst);
        m_Layout.toolsViewportSplitter->SetFixedFirstWidth(paneWidth);
    }
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
    entry.homeZone = (zone == ::we::editor::docking::DockZone::Floating)
        ? ::we::editor::docking::DockZone::Right
        : zone;
    entry.visible = panel->IsVisible();
    entry.floating = false;
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

    if (it->second.floating && !visible) {
        if (m_PopupHost && it->second.floatFrame) {
            m_PopupHost->ClosePopup(it->second.floatFrame);
        }
        if (it->second.floatFrame) {
            (void)it->second.floatFrame->TakePanel();
            it->second.floatFrame.reset();
        }
        it->second.panel->SetHeaderHeight(0.0f);
        it->second.floating = false;
        it->second.zone = it->second.homeZone;
        it->second.visible = false;
        it->second.panel->SetVisible(false);
        if (m_OnPanelVisibilityChanged) {
            m_OnPanelVisibilityChanged();
        }
        we::runtime::kindui::UIRepaintGate::RequestLayout();
        return;
    }

    it->second.visible = visible;
    it->second.panel->SetVisible(visible);

    if (it->second.floating) {
        if (m_OnPanelVisibilityChanged) {
            m_OnPanelVisibilityChanged();
        }
        we::runtime::kindui::UIRepaintGate::RequestLayout();
        return;
    }

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

    UpdateEmptyDockVisibility();

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

std::string EditorWorkspaceController::FindPanelId(const ::we::editor::panels::Panel* panel) const {
    if (!panel) {
        return {};
    }
    for (const auto& [id, entry] : m_Panels) {
        if (entry.panel.get() == panel) {
            return id;
        }
    }
    return {};
}

void EditorWorkspaceController::ShowFloatingOptionsMenu(const std::string& panelId) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel || !m_PopupHost || !it->second.floatFrame) {
        return;
    }

    const Rect header = it->second.floatFrame->GetGeometry();
    const float titleH = we::runtime::kindui::ResolveMetric(
        we::runtime::kindui::MetricToken::TitleBarHeight)
        * ::we::editor::panels::PanelChrome::UiScale();

    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;

    auto dockItem = std::make_shared<::we::editor::menus::MenuItem>();
    dockItem->label = "Dock Panel";
    dockItem->enabled = true;
    dockItem->onClick = [panelId]() {
        EditorWorkspaceController::Get().DockPanel(panelId);
    };
    items.push_back(dockItem);

    auto closeItem = std::make_shared<::we::editor::menus::MenuItem>();
    closeItem->label = "Close Panel";
    closeItem->enabled = true;
    closeItem->onClick = [panelId]() {
        EditorWorkspaceController::Get().SetPanelVisible(panelId, false);
    };
    items.push_back(closeItem);

    auto menu = std::make_shared<::we::editor::menus::DropdownMenu>(items);
    m_PopupHost->CloseTransientPopups();
    const float menuY = header.y + titleH + 2.0f;
    m_PopupHost->ShowPopup(menu, we::runtime::kindui::Point{
        header.x + header.width - 160.0f,
        menuY
    });
}

::we::editor::docking::DockZone EditorWorkspaceController::ZoneForDock(
    const std::shared_ptr<::we::editor::docking::DockContainer>& dock) const {
    if (!dock) {
        return ::we::editor::docking::DockZone::Floating;
    }
    if (dock == m_Layout.toolsDock) {
        return ::we::editor::docking::DockZone::Left;
    }
    if (dock == m_Layout.viewportDock) {
        return ::we::editor::docking::DockZone::Center;
    }
    if (dock == m_Layout.explorerDock || dock == m_Layout.detailsDock) {
        return ::we::editor::docking::DockZone::Right;
    }
    if (dock == m_Layout.contentBrowserDock) {
        return ::we::editor::docking::DockZone::Bottom;
    }
    return ::we::editor::docking::DockZone::Floating;
}

void EditorWorkspaceController::BeginFloating(
    PanelEntry& entry,
    const std::string& panelId,
    const we::runtime::kindui::Point& position) {
    if (!entry.panel || !m_PopupHost) {
        return;
    }

    if (entry.floating && entry.floatFrame) {
        m_PopupHost->MovePopup(entry.floatFrame, position);
        return;
    }

    if (entry.zone != ::we::editor::docking::DockZone::Floating) {
        entry.homeZone = entry.zone;
    }

    if (auto dock = DockForPanel(panelId)) {
        dock->RemovePanel(entry.panel);
    } else if (auto zoneDock = DockForZone(entry.zone)) {
        zoneDock->RemovePanel(entry.panel);
    }

    const Rect geom = entry.panel->GetGeometry();
    const float titleH = we::runtime::kindui::ResolveMetric(
        we::runtime::kindui::MetricToken::TitleBarHeight)
        * ::we::editor::panels::PanelChrome::UiScale();
    we::runtime::kindui::Size floatSize{
        (std::max)(geom.width, 320.0f),
        (std::max)(geom.height + titleH, 280.0f)
    };
    if (floatSize.width < 40.0f) {
        floatSize.width = 360.0f;
    }
    if (floatSize.height < 40.0f) {
        floatSize.height = 420.0f;
    }

    auto frame = std::make_shared<::we::editor::docking::FloatingPanelFrame>();
    frame->SetPanel(entry.panel);
    if (m_PopupHost) {
        frame->SetWorkspaceBounds(m_PopupHost->GetGeometry());
    }

    frame->SetOnClose([panelId]() {
        EditorWorkspaceController::Get().SetPanelVisible(panelId, false);
    });

    frame->SetOnResize([this, weak = std::weak_ptr<::we::editor::docking::FloatingPanelFrame>(frame)](
        const Rect& bounds) {
        if (!m_PopupHost) {
            return;
        }
        auto host = weak.lock();
        if (!host) {
            return;
        }
        m_PopupHost->ResizePopup(host, bounds);
        we::runtime::kindui::UIRepaintGate::RequestPaint();
    });

    frame->SetOnMove([this, weak = std::weak_ptr<::we::editor::docking::FloatingPanelFrame>(frame)](
        const we::runtime::kindui::Point& delta) {
        if (!m_PopupHost) {
            return;
        }
        auto host = weak.lock();
        if (!host) {
            return;
        }
        const Rect g = host->GetGeometry();
        m_PopupHost->MovePopup(host, we::runtime::kindui::Point{ g.x + delta.x, g.y + delta.y });
        we::runtime::kindui::UIRepaintGate::RequestPaint();
    });

    m_PopupHost->CloseTransientPopups();
    m_PopupHost->ShowPinnedPopup(frame, position, floatSize);

    entry.floatFrame = std::move(frame);
    entry.zone = ::we::editor::docking::DockZone::Floating;
    entry.floating = true;
    entry.visible = true;
    entry.panel->SetVisible(true);

    UpdateEmptyDockVisibility();
    we::runtime::kindui::UIRepaintGate::RequestLayout();
}

void EditorWorkspaceController::FloatPanel(const std::string& panelId) {
    FloatPanelAt(panelId, we::runtime::kindui::Point{ 120.0f, 100.0f });
}

void EditorWorkspaceController::FloatPanelAt(
    const std::string& panelId,
    const we::runtime::kindui::Point& position) {
    if (panelId.empty()) {
        return;
    }
    // Defer: callers often run inside DockContainer::OnMouseMove or menu callbacks.
    m_PendingFloatId = panelId;
    m_PendingFloatPos = position;
    m_PendingDockId.clear();
}

void EditorWorkspaceController::FloatPanelWidget(
    const std::shared_ptr<::we::editor::panels::Panel>& panel) {
    if (!panel) {
        return;
    }
    const Rect g = panel->GetGeometry();
    FloatPanelWidget(panel, we::runtime::kindui::Point{ g.x + 24.0f, g.y + 24.0f });
}

void EditorWorkspaceController::FloatPanelWidget(
    const std::shared_ptr<::we::editor::panels::Panel>& panel,
    const we::runtime::kindui::Point& position) {
    const std::string id = FindPanelId(panel.get());
    if (id.empty()) {
        return;
    }
    FloatPanelAt(id, position);
}

void EditorWorkspaceController::HidePanelWidget(
    const std::shared_ptr<::we::editor::panels::Panel>& panel) {
    const std::string id = FindPanelId(panel.get());
    if (id.empty()) {
        return;
    }
    SetPanelVisible(id, false);
}

void EditorWorkspaceController::DockPanel(const std::string& panelId) {
    if (panelId.empty()) {
        return;
    }
    m_PendingDockId = panelId;
    m_PendingDockTarget.reset();
    m_PendingFloatId.clear();
}

void EditorWorkspaceController::DockPanelTo(
    const std::string& panelId,
    const std::shared_ptr<::we::editor::docking::DockContainer>& targetDock) {
    if (panelId.empty()) {
        return;
    }
    m_PendingDockId = panelId;
    m_PendingDockTarget = targetDock;
    m_PendingFloatId.clear();
}

void EditorWorkspaceController::FlushPendingDockActions() {
    if (!m_PendingFloatId.empty()) {
        const std::string id = m_PendingFloatId;
        const Point pos = m_PendingFloatPos;
        m_PendingFloatId.clear();
        const auto it = m_Panels.find(id);
        if (it != m_Panels.end()) {
            BeginFloating(it->second, id, pos);
        }
    }

    if (!m_PendingDockId.empty()) {
        const std::string id = m_PendingDockId;
        auto target = m_PendingDockTarget;
        m_PendingDockId.clear();
        m_PendingDockTarget.reset();
        ApplyDockPanel(id, target);
    }
}

void EditorWorkspaceController::ApplyDockPanel(
    const std::string& panelId,
    const std::shared_ptr<::we::editor::docking::DockContainer>& targetDock) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }

    PanelEntry& entry = it->second;
    if (!entry.floating) {
        return;
    }

    if (m_PopupHost && entry.floatFrame) {
        m_PopupHost->ClosePopup(entry.floatFrame);
    }

    if (entry.floatFrame) {
        (void)entry.floatFrame->TakePanel();
        entry.floatFrame.reset();
    }

    entry.panel->SetHeaderHeight(0.0f);
    entry.floating = false;
    entry.visible = true;
    entry.panel->SetVisible(true);

    auto dock = targetDock;
    if (!dock) {
        dock = DockForPanel(panelId);
    }
    if (!dock) {
        dock = DockForZone(entry.homeZone);
    }

    if (dock) {
        entry.zone = ZoneForDock(dock);
        if (entry.zone != ::we::editor::docking::DockZone::Floating) {
            entry.homeZone = entry.zone;
        }
        // Make sure empty/hidden docks become visible before attach.
        dock->SetVisible(true);
        if (dock == m_Layout.contentBrowserDock && !m_ContentBrowserExpanded) {
            m_ContentBrowserExpanded = true;
            if (m_Layout.rootVerticalSplitter) {
                m_Layout.rootVerticalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
                m_Layout.rootVerticalSplitter->SetFixedSecondWidth(
                    SanitizeContentBrowserHeight(m_ContentBrowserBottomHeight));
            }
        }
        if (!dock->ContainsPanel(entry.panel)) {
            dock->AddPanel(entry.panel);
        }
        dock->FocusPanel(entry.panel);
    } else {
        entry.zone = entry.homeZone;
    }

    UpdateEmptyDockVisibility();
    we::runtime::kindui::UIRepaintGate::RequestLayout();
}

void EditorWorkspaceController::UpdateEmptyDockVisibility() {
    auto dockHasTabs = [](const std::shared_ptr<::we::editor::docking::DockContainer>& dock) {
        return dock && dock->GetTabCount() > 0;
    };

    if (m_Layout.explorerDock) {
        m_Layout.explorerDock->SetVisible(dockHasTabs(m_Layout.explorerDock));
    }
    if (m_Layout.detailsDock) {
        m_Layout.detailsDock->SetVisible(dockHasTabs(m_Layout.detailsDock));
    }
    if (m_Layout.contentBrowserDock) {
        const bool show = dockHasTabs(m_Layout.contentBrowserDock) && m_ContentBrowserExpanded;
        m_Layout.contentBrowserDock->SetVisible(show);
    }

    const bool rightVisible =
        (m_Layout.explorerDock && m_Layout.explorerDock->IsVisible())
        || (m_Layout.detailsDock && m_Layout.detailsDock->IsVisible());

    if (m_Layout.rightVerticalSplitter) {
        m_Layout.rightVerticalSplitter->SetVisible(rightVisible);
    }

    if (m_Layout.mainHorizontalSplitter) {
        m_Layout.mainHorizontalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
        if (rightVisible) {
            const float width = m_RightSidebarWidth > 0.0f ? m_RightSidebarWidth : 340.0f;
            m_Layout.mainHorizontalSplitter->SetFixedSecondWidth(std::max(width, 280.0f));
        } else {
            const float current = m_Layout.mainHorizontalSplitter->GetFixedSecondWidth();
            if (current >= 200.0f) {
                m_RightSidebarWidth = current;
            }
            m_Layout.mainHorizontalSplitter->SetFixedSecondWidth(0.0f);
        }
    }
}

void EditorWorkspaceController::EnsureDefaultDockPlacement() {
    m_PendingFloatId.clear();
    m_PendingDockId.clear();
    m_PendingDockTarget.reset();

    static const char* kCorePanels[] = {
        "Tools", "Viewport", "WorldOutliner", "Details", "ContentBrowser"
    };

    for (const char* panelId : kCorePanels) {
        const auto it = m_Panels.find(panelId);
        if (it == m_Panels.end() || !it->second.panel) {
            continue;
        }

        PanelEntry& entry = it->second;
        if (entry.floating) {
            if (m_PopupHost && entry.floatFrame) {
                m_PopupHost->ClosePopup(entry.floatFrame);
            }
            if (entry.floatFrame) {
                (void)entry.floatFrame->TakePanel();
                entry.floatFrame.reset();
            }
            entry.floating = false;
        }

        entry.panel->SetHeaderHeight(0.0f);
        entry.zone = entry.homeZone;
        entry.visible = true;
        entry.panel->SetVisible(true);

        if (auto dock = DockForPanel(panelId)) {
            dock->SetVisible(true);
            if (!dock->ContainsPanel(entry.panel)) {
                dock->AddPanel(entry.panel);
            }
            dock->FocusPanel(entry.panel);
        }
    }

    if (m_Layout.explorerDock) {
        m_Layout.explorerDock->SetVisible(true);
    }
    if (m_Layout.detailsDock) {
        m_Layout.detailsDock->SetVisible(true);
    }
    if (m_Layout.contentBrowserDock) {
        m_Layout.contentBrowserDock->SetVisible(m_ContentBrowserExpanded);
    }
    if (m_Layout.rightVerticalSplitter) {
        m_Layout.rightVerticalSplitter->SetVisible(true);
    }
    if (m_Layout.mainHorizontalSplitter) {
        m_Layout.mainHorizontalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
        const float width = m_RightSidebarWidth > 0.0f ? m_RightSidebarWidth : 340.0f;
        m_Layout.mainHorizontalSplitter->SetFixedSecondWidth(std::max(width, 280.0f));
    }

    UpdateEmptyDockVisibility();
    we::runtime::kindui::UIRepaintGate::RequestLayout();
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
            ApplyToolsPaneWidth(m_ToolsPaneWidth > 0.0f ? m_ToolsPaneWidth : 300.0f);
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
            ApplyToolsPaneWidth(parsed);
        } else if (key == "mainHorizontalRightWidth" && m_Layout.mainHorizontalSplitter) {
            m_RightSidebarWidth = std::max(parsed, 280.0f);
            m_Layout.mainHorizontalSplitter->SetResizeMode(Splitter::ResizeMode::FixedSecond);
            m_Layout.mainHorizontalSplitter->SetFixedSecondWidth(m_RightSidebarWidth);
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
            ApplyToolsPaneWidth(parsed);
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
    if (m_Layout.mainHorizontalSplitter
        && m_Layout.mainHorizontalSplitter->GetResizeMode() == Splitter::ResizeMode::FixedSecond) {
        const float right = m_Layout.mainHorizontalSplitter->GetFixedSecondWidth();
        if (right >= 200.0f) {
            file << "mainHorizontalRightWidth=" << right << "\n";
        } else if (m_RightSidebarWidth >= 200.0f) {
            file << "mainHorizontalRightWidth=" << m_RightSidebarWidth << "\n";
        }
    }
    writeRatio("rightVertical", m_Layout.rightVerticalSplitter);
}

void EditorWorkspaceController::Reset() {
    m_PendingFloatId.clear();
    m_PendingDockId.clear();
    m_PendingDockTarget.reset();

    for (auto& [panelId, entry] : m_Panels) {
        (void)panelId;
        if (entry.floatFrame) {
            if (m_PopupHost) {
                m_PopupHost->ClosePopup(entry.floatFrame);
            }
            (void)entry.floatFrame->TakePanel();
            entry.floatFrame.reset();
        }
        entry.floating = false;
    }

    m_Panels.clear();
    m_PopupHost = nullptr;
    m_OnPanelVisibilityChanged = nullptr;
    // Keep m_Layout alive until ClearLayoutRefs() after the overlay tree is destroyed.
}

void EditorWorkspaceController::ClearLayoutRefs() {
    m_Layout = {};
}

} // namespace we::programs::editor
