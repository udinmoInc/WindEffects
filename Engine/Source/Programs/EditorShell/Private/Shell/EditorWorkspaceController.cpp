#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"

#include "Core/EditorConfigPaths.h"
#include "Core/Logger.h"
#include "KindUI/Panel/Panel.h"
#include "KindUI/Docking/DockContainer.h"
#include "KindUI/Docking/FloatingPanelFrame.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Profiling/PaintCauseLog.h"
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
    const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel,
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

std::shared_ptr<::we::runtime::kindui::docking::DockContainer> EditorWorkspaceController::DockForPanel(
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

std::shared_ptr<::we::runtime::kindui::docking::DockContainer> EditorWorkspaceController::DockForZone(
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
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;

    if (it->second.floating && !visible) {
        DetachPanelFromFloatHost(it->second);
        it->second.panel->SetHeaderHeight(0.0f);
        it->second.floating = false;
        it->second.floatHostId = -1;
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
        if (visible) {
            if (FloatingHost* host = FindFloatingHost(it->second.floatHostId)) {
                host->dock->FocusPanel(it->second.panel);
            }
        }
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

std::string EditorWorkspaceController::FindPanelId(const ::we::runtime::kindui::panels::Panel* panel) const {
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

EditorWorkspaceController::FloatingHost* EditorWorkspaceController::FindFloatingHost(int hostId) {
    if (hostId < 0) {
        return nullptr;
    }
    for (auto& host : m_FloatHosts) {
        if (host.id == hostId) {
            return &host;
        }
    }
    return nullptr;
}

const EditorWorkspaceController::FloatingHost* EditorWorkspaceController::FindFloatingHost(int hostId) const {
    if (hostId < 0) {
        return nullptr;
    }
    for (const auto& host : m_FloatHosts) {
        if (host.id == hostId) {
            return &host;
        }
    }
    return nullptr;
}

EditorWorkspaceController::FloatingHost* EditorWorkspaceController::FindFloatingHostByDock(
    const ::we::runtime::kindui::docking::DockContainer* dock) {
    if (!dock) {
        return nullptr;
    }
    for (auto& host : m_FloatHosts) {
        if (host.dock.get() == dock) {
            return &host;
        }
    }
    return nullptr;
}

EditorWorkspaceController::FloatingHost* EditorWorkspaceController::FindFloatingHostAtTabStrip(
    const we::runtime::kindui::Point& cursor,
    int excludeHostId) {
    FloatingHost* best = nullptr;
    float bestArea = 1.0e30f;
    for (auto& host : m_FloatHosts) {
        if (host.id == excludeHostId || !host.dock) {
            continue;
        }
        const Rect strip = host.dock->GetTabStripRect();
        if (strip.width < 4.0f || strip.height < 4.0f || !strip.Contains(cursor)) {
            continue;
        }
        const float area = strip.width * strip.height;
        if (area < bestArea) {
            bestArea = area;
            best = &host;
        }
    }
    return best;
}

std::shared_ptr<::we::runtime::kindui::docking::DockContainer> EditorWorkspaceController::FindZoneDockAtTabStrip(
    const we::runtime::kindui::Point& cursor) const {
    struct Candidate {
        std::shared_ptr<::we::runtime::kindui::docking::DockContainer> dock;
        float area = 0.0f;
    };
    std::vector<Candidate> candidates;

    auto consider = [&](const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& dock) {
        if (!dock || !dock->IsVisible() || dock->GetTabCount() < 0) {
            return;
        }
        // Accept empty visible docks? Prefer docks with tabs or still-visible geometry.
        const Rect strip = dock->GetTabStripRect();
        if (strip.width < 4.0f || strip.height < 4.0f || !strip.Contains(cursor)) {
            return;
        }
        candidates.push_back(Candidate{ dock, strip.width * strip.height });
    };

    consider(m_Layout.toolsDock);
    consider(m_Layout.explorerDock);
    consider(m_Layout.detailsDock);
    consider(m_Layout.contentBrowserDock);
    consider(m_Layout.viewportDock);

    if (candidates.empty()) {
        return nullptr;
    }
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.area < b.area;
    });
    return candidates.front().dock;
}

void EditorWorkspaceController::DestroyFloatingHost(int hostId) {
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;
    for (size_t i = 0; i < m_FloatHosts.size(); ++i) {
        if (m_FloatHosts[i].id != hostId) {
            continue;
        }
        FloatingHost& host = m_FloatHosts[i];
        if (m_PopupHost && host.frame) {
            m_PopupHost->ClosePopup(host.frame);
        }
        if (host.frame) {
            (void)host.frame->TakeDock();
        }
        m_FloatHosts.erase(m_FloatHosts.begin() + static_cast<std::ptrdiff_t>(i));
        return;
    }
}

void EditorWorkspaceController::DetachPanelFromFloatHost(PanelEntry& entry) {
    if (!entry.floating || entry.floatHostId < 0 || !entry.panel) {
        return;
    }
    FloatingHost* host = FindFloatingHost(entry.floatHostId);
    if (!host || !host->dock) {
        entry.floatHostId = -1;
        entry.floating = false;
        return;
    }

    if (host->dock->ContainsPanel(entry.panel)) {
        host->dock->RemovePanel(entry.panel);
    }
    const int hostId = host->id;
    entry.floatHostId = -1;
    entry.floating = false;

    if (host->dock->GetTabCount() <= 0) {
        DestroyFloatingHost(hostId);
    }
}

void EditorWorkspaceController::WireFloatingDock(FloatingHost& host) {
    const int hostId = host.id;
    host.dock->SetOnTabClosed([this](const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel) {
        const std::string id = FindPanelId(panel.get());
        if (!id.empty()) {
            SetPanelVisible(id, false);
        }
    });
    host.dock->SetOnTabDragStarted([this, hostId](
        const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel,
        const Point& pos) {
        const std::string id = FindPanelId(panel.get());
        if (id.empty()) {
            return;
        }
        // Peel tab into a new float / merge / redock based on drop cursor.
        FloatPanelAt(id, pos);
        // Stash source host so BeginFloating can decide peel vs move.
        (void)hostId;
    });
}

EditorWorkspaceController::FloatingHost& EditorWorkspaceController::CreateFloatingHost(
    const we::runtime::kindui::Point& position,
    const we::runtime::kindui::Size& size) {
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;
    FloatingHost host;
    host.id = m_NextFloatHostId++;
    host.dock = std::make_shared<::we::runtime::kindui::docking::DockContainer>();
    host.dock->SetHeaderHeightLogical(
        we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PanelTabHeight));

    host.frame = std::make_shared<::we::runtime::kindui::docking::FloatingPanelFrame>();
    host.frame->SetDock(host.dock);
    if (m_PopupHost) {
        host.frame->SetWorkspaceBounds(m_PopupHost->GetGeometry());
    }

    const int hostId = host.id;
    host.frame->SetOnClose([this, hostId]() {
        // Close all panels in this floating window.
        FloatingHost* h = FindFloatingHost(hostId);
        if (!h || !h->dock) {
            return;
        }
        const auto panels = h->dock->GetPanels();
        for (const auto& panel : panels) {
            const std::string id = FindPanelId(panel.get());
            if (!id.empty()) {
                SetPanelVisible(id, false);
            }
        }
    });

    host.frame->SetOnResize([this, hostId](const Rect& bounds) {
        if (!m_PopupHost) {
            return;
        }
        FloatingHost* h = FindFloatingHost(hostId);
        if (!h || !h->frame) {
            return;
        }
        m_PopupHost->ResizePopup(h->frame, bounds);
        we::runtime::kindui::UIRepaintGate::RequestPaint();
    });

    host.frame->SetOnMove([this, hostId](const Point& delta) {
        if (!m_PopupHost) {
            return;
        }
        FloatingHost* h = FindFloatingHost(hostId);
        if (!h || !h->frame) {
            return;
        }
        const Rect g = h->frame->GetGeometry();
        m_PopupHost->MovePopup(h->frame, Point{ g.x + delta.x, g.y + delta.y });
        we::runtime::kindui::UIRepaintGate::RequestPaint();
    });

    WireFloatingDock(host);

    m_PopupHost->CloseTransientPopups();
    m_PopupHost->ShowPinnedPopup(host.frame, position, size);

    m_FloatHosts.push_back(std::move(host));
    ::we::runtime::kindui::PaintCauseLog::Get().Push(
        "float-create", WE_PAINT_CALLER);
    return m_FloatHosts.back();
}

void EditorWorkspaceController::ShowFloatingOptionsMenu(const std::string& panelId) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel || !m_PopupHost) {
        return;
    }
    FloatingHost* host = FindFloatingHost(it->second.floatHostId);
    if (!host || !host->frame) {
        return;
    }

    const Rect header = host->frame->GetGeometry();
    const float titleH = host->dock ? host->dock->GetHeaderHeightDevice()
        : we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PanelTabHeight)
            * ::we::runtime::kindui::panels::PanelChrome::UiScale();

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
    m_PopupHost->ShowPopup(menu, Point{
        header.x + header.width - 160.0f,
        header.y + titleH + 2.0f
    });
}

::we::editor::docking::DockZone EditorWorkspaceController::ZoneForDock(
    const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& dock) const {
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
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;

    // Already floating: resolve drop target under cursor (merge / redock / move host).
    if (entry.floating) {
        const int sourceHostId = entry.floatHostId;
        FloatingHost* sourceHost = FindFloatingHost(sourceHostId);

        if (auto zoneDock = FindZoneDockAtTabStrip(position)) {
            ApplyDockPanel(panelId, zoneDock);
            return;
        }

        if (FloatingHost* targetHost = FindFloatingHostAtTabStrip(position, sourceHostId)) {
            if (sourceHost && sourceHost->dock && sourceHost->dock->ContainsPanel(entry.panel)) {
                sourceHost->dock->RemovePanel(entry.panel);
            }
            if (!targetHost->dock->ContainsPanel(entry.panel)) {
                targetHost->dock->AddPanel(entry.panel);
            }
            targetHost->dock->FocusPanel(entry.panel);
            entry.floatHostId = targetHost->id;
            entry.floating = true;
            entry.zone = ::we::editor::docking::DockZone::Floating;
            if (sourceHost && sourceHost->dock && sourceHost->dock->GetTabCount() <= 0) {
                DestroyFloatingHost(sourceHostId);
            }
            we::runtime::kindui::UIRepaintGate::RequestLayout();
            return;
        }

        // Peel into a new floating window when the source host still has other tabs.
        if (sourceHost && sourceHost->dock && sourceHost->dock->GetTabCount() > 1) {
            sourceHost->dock->RemovePanel(entry.panel);
            const Rect geom = sourceHost->frame ? sourceHost->frame->GetGeometry() : Rect{};
            Size floatSize{
                (std::max)(geom.width, 320.0f),
                (std::max)(geom.height, 280.0f)
            };
            FloatingHost& newHost = CreateFloatingHost(position, floatSize);
            newHost.dock->AddPanel(entry.panel);
            newHost.dock->FocusPanel(entry.panel);
            entry.floatHostId = newHost.id;
            entry.floating = true;
            entry.zone = ::we::editor::docking::DockZone::Floating;
            we::runtime::kindui::UIRepaintGate::RequestLayout();
            return;
        }

        // Sole tab: move the existing floating window.
        if (sourceHost && sourceHost->frame) {
            m_PopupHost->MovePopup(sourceHost->frame, position);
            we::runtime::kindui::UIRepaintGate::RequestPaint();
        }
        return;
    }

    if (entry.zone != ::we::editor::docking::DockZone::Floating) {
        entry.homeZone = entry.zone;
    }

    // Prefer merging into an existing floating window under the cursor.
    if (FloatingHost* targetHost = FindFloatingHostAtTabStrip(position)) {
        if (auto dock = DockForPanel(panelId)) {
            dock->RemovePanel(entry.panel);
        } else if (auto zoneDock = DockForZone(entry.zone)) {
            zoneDock->RemovePanel(entry.panel);
        }
        if (!targetHost->dock->ContainsPanel(entry.panel)) {
            targetHost->dock->AddPanel(entry.panel);
        }
        targetHost->dock->FocusPanel(entry.panel);
        entry.floatHostId = targetHost->id;
        entry.zone = ::we::editor::docking::DockZone::Floating;
        entry.floating = true;
        entry.visible = true;
        entry.panel->SetVisible(true);
        UpdateEmptyDockVisibility();
        we::runtime::kindui::UIRepaintGate::RequestLayout();
        return;
    }

    // Redock onto a zone tab strip if the undock drag lands there.
    if (auto zoneDock = FindZoneDockAtTabStrip(position)) {
        if (auto dock = DockForPanel(panelId)) {
            if (dock != zoneDock) {
                dock->RemovePanel(entry.panel);
            }
        } else if (auto home = DockForZone(entry.zone)) {
            if (home != zoneDock) {
                home->RemovePanel(entry.panel);
            }
        }
        zoneDock->SetVisible(true);
        if (!zoneDock->ContainsPanel(entry.panel)) {
            zoneDock->AddPanel(entry.panel);
        }
        zoneDock->FocusPanel(entry.panel);
        entry.zone = ZoneForDock(zoneDock);
        if (entry.zone != ::we::editor::docking::DockZone::Floating) {
            entry.homeZone = entry.zone;
        }
        entry.floating = false;
        entry.floatHostId = -1;
        entry.visible = true;
        entry.panel->SetVisible(true);
        UpdateEmptyDockVisibility();
        we::runtime::kindui::UIRepaintGate::RequestLayout();
        return;
    }

    if (auto dock = DockForPanel(panelId)) {
        dock->RemovePanel(entry.panel);
    } else if (auto zoneDock = DockForZone(entry.zone)) {
        zoneDock->RemovePanel(entry.panel);
    }

    const Rect geom = entry.panel->GetGeometry();
    const float tabH = we::runtime::kindui::ResolveMetric(
        we::runtime::kindui::MetricToken::PanelTabHeight)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
    Size floatSize{
        (std::max)(geom.width, 320.0f),
        (std::max)(geom.height + tabH, 280.0f)
    };
    if (floatSize.width < 40.0f) {
        floatSize.width = 360.0f;
    }
    if (floatSize.height < 40.0f) {
        floatSize.height = 420.0f;
    }

    FloatingHost& host = CreateFloatingHost(position, floatSize);
    host.dock->AddPanel(entry.panel);
    host.dock->FocusPanel(entry.panel);

    entry.floatHostId = host.id;
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
    m_PendingFloatId = panelId;
    m_PendingFloatPos = position;
    m_PendingDockId.clear();
}

void EditorWorkspaceController::FloatPanelWidget(
    const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel) {
    if (!panel) {
        return;
    }
    const Rect g = panel->GetGeometry();
    FloatPanelWidget(panel, we::runtime::kindui::Point{ g.x + 24.0f, g.y + 24.0f });
}

void EditorWorkspaceController::FloatPanelWidget(
    const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel,
    const we::runtime::kindui::Point& position) {
    const std::string id = FindPanelId(panel.get());
    if (id.empty()) {
        return;
    }
    FloatPanelAt(id, position);
}

void EditorWorkspaceController::HidePanelWidget(
    const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel) {
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
    const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& targetDock) {
    if (panelId.empty()) {
        return;
    }
    m_PendingDockId = panelId;
    m_PendingDockTarget = targetDock;
    m_PendingFloatId.clear();
}

void EditorWorkspaceController::FlushPendingDockActions() {
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;
    bool hadAction = !m_PendingFloatId.empty() || !m_PendingDockId.empty();
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

    if (hadAction && m_EventSystem) {
        m_EventSystem->ClearAllInputState();
    }
}

void EditorWorkspaceController::ApplyDockPanel(
    const std::string& panelId,
    const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& targetDock) {
    const auto it = m_Panels.find(panelId);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;

    PanelEntry& entry = it->second;
    if (!entry.floating) {
        return;
    }

    DetachPanelFromFloatHost(entry);

    entry.panel->SetHeaderHeight(0.0f);
    entry.floating = false;
    entry.floatHostId = -1;
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
    auto dockHasTabs = [](const std::shared_ptr<::we::runtime::kindui::docking::DockContainer>& dock) {
        return dock && dock->GetTabCount() > 0;
    };

    const bool drawerVisible = ::we::editor::shell::EditorModeController::Get().IsDrawerVisible();
    if (m_Layout.toolsDock) {
        const bool showTools = dockHasTabs(m_Layout.toolsDock) && drawerVisible;
        m_Layout.toolsDock->SetVisible(showTools);
        if (auto toolsIt = m_Panels.find("Tools"); toolsIt != m_Panels.end() && toolsIt->second.panel) {
            toolsIt->second.panel->SetVisible(showTools);
            toolsIt->second.visible = showTools;
        }
    }
    if (m_Layout.viewportDock) {
        m_Layout.viewportDock->SetVisible(dockHasTabs(m_Layout.viewportDock));
    }
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

    for (auto& host : m_FloatHosts) {
        if (host.dock && host.frame) {
            host.frame->SetVisible(dockHasTabs(host.dock));
        }
    }

    const bool rightVisible =
        (m_Layout.explorerDock && m_Layout.explorerDock->IsVisible())
        || (m_Layout.detailsDock && m_Layout.detailsDock->IsVisible());

    if (m_Layout.rightVerticalSplitter) {
        m_Layout.rightVerticalSplitter->SetVisible(rightVisible);
    }
}

void EditorWorkspaceController::EnsureDefaultDockPlacement() {
    m_PendingFloatId.clear();
    m_PendingDockId.clear();
    m_PendingDockTarget.reset();
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;

    // Tear down all floating hosts first.
    while (!m_FloatHosts.empty()) {
        DestroyFloatingHost(m_FloatHosts.front().id);
    }

    static const char* kCorePanels[] = {
        "Tools", "Viewport", "WorldOutliner", "Details", "ContentBrowser"
    };

    for (const char* panelId : kCorePanels) {
        const auto it = m_Panels.find(panelId);
        if (it == m_Panels.end() || !it->second.panel) {
            continue;
        }

        PanelEntry& entry = it->second;
        entry.floating = false;
        entry.floatHostId = -1;
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

    if (it->second.floating) {
        if (FloatingHost* host = FindFloatingHost(it->second.floatHostId)) {
            host->dock->FocusPanel(it->second.panel);
        }
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
    we::runtime::kindui::UIRepaintGate::ScopedBatch batch;

    while (!m_FloatHosts.empty()) {
        DestroyFloatingHost(m_FloatHosts.front().id);
    }

    for (auto& [panelId, entry] : m_Panels) {
        (void)panelId;
        entry.floating = false;
        entry.floatHostId = -1;
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
