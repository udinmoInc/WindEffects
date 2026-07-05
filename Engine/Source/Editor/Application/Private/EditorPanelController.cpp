#include "EditorPanelController.hpp"

#include "EditorLayoutController.hpp"
#include "Widgets/DockContainer.hpp"
#include "Widgets/Panel.hpp"
#include "Layout/OverlayManager.hpp"
#include "Core/Theme.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Icon.hpp"
#include "Core/Logger.hpp"

namespace we::programs::editor {

namespace {

class FloatingPanelHost : public we::UI::Widget {
public:
    FloatingPanelHost(const std::shared_ptr<we::UI::Panel>& panel, std::function<void()> onClose)
        : m_Panel(panel), m_OnClose(std::move(onClose)) {
        AddChild(panel);
        m_Title = panel ? panel->GetTitle() : "Panel";
    }

    we::UI::Size Measure(const we::UI::Size& availableSize) override {
        const float width = std::min(availableSize.width * 0.45f, 520.0f);
        const float height = std::min(availableSize.height * 0.55f, 640.0f);
        m_DesiredSize = we::UI::Size{ width, height };
        return m_DesiredSize;
    }

    void Arrange(const we::UI::Rect& allottedRect) override {
        m_Geometry = allottedRect;
        constexpr float kHeaderHeight = 28.0f;
        m_HeaderRect = we::UI::Rect{ allottedRect.x, allottedRect.y, allottedRect.width, kHeaderHeight };
        m_ContentRect = we::UI::Rect{
            allottedRect.x,
            allottedRect.y + kHeaderHeight,
            allottedRect.width,
            allottedRect.height - kHeaderHeight
        };
        if (m_Panel) {
            m_Panel->Arrange(m_ContentRect);
        }
    }

    void Paint(we::UI::PaintContext& context) override {
        const auto& theme = we::UI::Theme::Get();
        context.DrawShadow(m_Geometry, we::UI::Color{ 0.0f, 0.0f, 0.0f, 0.35f }, 8.0f, 16.0f);
        context.DrawRoundedRect(m_Geometry, theme.PanelBackground, theme.CornerRadiusMedium);
        context.DrawRoundedRectOutline(m_Geometry, theme.BorderDefault, 1.0f, theme.CornerRadiusMedium);
        context.DrawRect(m_HeaderRect, theme.HeaderBackground);
        context.DrawRect(
            we::UI::Rect{ m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, m_HeaderRect.width, 1.0f },
            theme.Separator);
        context.DrawText(m_Title, we::UI::Point{ m_HeaderRect.x + 10.0f, m_HeaderRect.y + 7.0f },
            theme.TextPrimary, theme.TextSizeTabs, true);

        constexpr float kCloseSize = 14.0f;
        m_CloseRect = we::UI::Rect{
            m_HeaderRect.x + m_HeaderRect.width - kCloseSize - 8.0f,
            m_HeaderRect.y + (m_HeaderRect.height - kCloseSize) * 0.5f,
            kCloseSize,
            kCloseSize
        };
        const we::UI::Color closeColor = m_CloseHovered ? theme.TextPrimary : theme.TextSecondary;
        we::UI::IconPainter::DrawIcon(context, we::UI::Icons::XName, m_CloseRect, closeColor);

        if (m_Panel) {
            if (auto content = m_Panel->GetContent()) {
                content->Paint(context);
            }
        }
    }

    void OnMouseDown(const we::UI::MouseEvent& event) override {
        if (event.button != we::UI::MouseButton::Left) {
            if (m_Panel && m_Panel->GetContent()) {
                m_Panel->GetContent()->OnMouseDown(event);
            }
            return;
        }

        if (m_CloseRect.Contains(event.position) && m_OnClose) {
            m_OnClose();
            return;
        }

        if (m_HeaderRect.Contains(event.position)) {
            m_Dragging = true;
            m_DragOffset = we::UI::Point{
                event.position.x - m_Geometry.x,
                event.position.y - m_Geometry.y
            };
            return;
        }

        if (m_Panel && m_Panel->GetContent() && m_ContentRect.Contains(event.position)) {
            m_Panel->GetContent()->OnMouseDown(event);
        }
    }

    void OnMouseMove(const we::UI::MouseEvent& event) override {
        m_CloseHovered = m_CloseRect.Contains(event.position);

        if (m_Dragging) {
            m_Geometry.x = event.position.x - m_DragOffset.x;
            m_Geometry.y = event.position.y - m_DragOffset.y;
            Arrange(m_Geometry);
            return;
        }

        if (m_Panel && m_Panel->GetContent() && m_ContentRect.Contains(event.position)) {
            m_Panel->GetContent()->OnMouseMove(event);
        }
    }

    void OnMouseUp(const we::UI::MouseEvent& event) override {
        m_Dragging = false;
        if (m_Panel && m_Panel->GetContent()) {
            m_Panel->GetContent()->OnMouseUp(event);
        }
    }

    void OnMouseWheel(const we::UI::MouseEvent& event) override {
        if (m_Panel && m_Panel->GetContent() && m_ContentRect.Contains(event.position)) {
            m_Panel->GetContent()->OnMouseWheel(event);
        }
    }

    bool ShowsPointerCursor(const we::UI::Point& position) const override {
        return m_HeaderRect.Contains(position) || m_CloseRect.Contains(position);
    }

private:
    std::shared_ptr<we::UI::Panel> m_Panel;
    std::function<void()> m_OnClose;
    std::string m_Title;
    we::UI::Rect m_HeaderRect;
    we::UI::Rect m_ContentRect;
    we::UI::Rect m_CloseRect;
    we::UI::Point m_DragOffset{};
    bool m_Dragging = false;
    bool m_CloseHovered = false;
};

bool UsesBottomSplitter(EditorPanelId id) {
    return id == EditorPanelId::ContentBrowser || id == EditorPanelId::Debug;
}

bool UsesFloatingPresentation(EditorPanelId id) {
    return id == EditorPanelId::ViewportNavigation || id == EditorPanelId::OutputLog;
}

void HideFloatingPanel(const std::shared_ptr<we::UI::Panel>& panel) {
    if (auto* overlay = we::UI::OverlayManager::Get()) {
        overlay->CloseAllPopups();
    }
    if (panel) {
        panel->SetVisible(false);
    }
}

} // namespace

EditorPanelController& EditorPanelController::Get() {
    static EditorPanelController instance;
    return instance;
}

void EditorPanelController::RegisterDockZone(EditorDockZone zone, const std::shared_ptr<we::UI::DockContainer>& dock) {
    m_DockZones[zone] = dock;
}

void EditorPanelController::RegisterPanel(EditorPanelId id,
                                        const std::string& menuLabel,
                                        const std::shared_ptr<we::UI::Panel>& panel,
                                        EditorDockZone defaultZone) {
    PanelEntry entry;
    entry.menuLabel = menuLabel;
    entry.panel = panel;
    entry.zone = defaultZone;
    entry.visible = true;
    m_Panels[id] = std::move(entry);
}

void EditorPanelController::SetPanelVisible(EditorPanelId id, bool visible) {
    auto it = m_Panels.find(id);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }

    auto& entry = it->second;
    entry.visible = visible;

    if (id == EditorPanelId::Details) {
        entry.panel->SetVisible(visible);
        if (m_OnVisibilityChanged) {
            m_OnVisibilityChanged();
        }
        return;
    }

    if (UsesBottomSplitter(id)) {
        entry.panel->SetVisible(visible);
        if (visible) {
            EditorLayoutController::Get().SetBottomPanelIndex(
                id == EditorPanelId::ContentBrowser ? 0 : 1);
        }
        if (m_OnVisibilityChanged) {
            m_OnVisibilityChanged();
        }
        return;
    }

    if (UsesFloatingPresentation(id)) {
        if (visible) {
            FloatPanel(id);
        } else {
            entry.floating = false;
            HideFloatingPanel(entry.panel);
        }
        if (m_OnVisibilityChanged) {
            m_OnVisibilityChanged();
        }
        return;
    }

    if (visible) {
        entry.floating = false;
        EnsurePanelInDock(entry);
        FocusPanel(id);
    } else {
        if (auto dock = GetDock(entry.zone)) {
            dock->RemovePanel(entry.panel);
        }
        entry.panel->SetVisible(false);
    }

    if (m_OnVisibilityChanged) {
        m_OnVisibilityChanged();
    }
}

void EditorPanelController::TogglePanelVisibility(EditorPanelId id) {
    SetPanelVisible(id, !IsPanelVisible(id));
}

bool EditorPanelController::IsPanelVisible(EditorPanelId id) const {
    auto it = m_Panels.find(id);
    if (it == m_Panels.end()) {
        return false;
    }
    return it->second.visible;
}

void EditorPanelController::FocusPanel(EditorPanelId id) {
    auto it = m_Panels.find(id);
    if (it == m_Panels.end() || !it->second.panel || !it->second.visible) {
        return;
    }

    it->second.panel->SetVisible(true);
    if (it->second.floating) {
        return;
    }

    if (auto dock = GetDock(it->second.zone)) {
        dock->FocusPanel(it->second.panel);
    }
}

void EditorPanelController::FloatPanel(EditorPanelId id) {
    auto it = m_Panels.find(id);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }

    auto& entry = it->second;
    if (auto dock = GetDock(entry.zone)) {
        dock->RemovePanel(entry.panel);
    } else if (auto parent = entry.panel->GetParent()) {
        parent->RemoveChild(entry.panel);
    }

    entry.floating = true;
    entry.visible = true;
    entry.panel->SetVisible(true);

    auto overlay = we::UI::OverlayManager::Get();
    if (!overlay) {
        HE_ERROR("[EditorPanel] OverlayManager unavailable for floating panel.");
        return;
    }

    overlay->CloseAllPopups();

    const EditorPanelId panelId = id;
    auto host = std::make_shared<FloatingPanelHost>(entry.panel, [this, panelId]() {
        SetPanelVisible(panelId, false);
        if (auto* overlayMgr = we::UI::OverlayManager::Get()) {
            overlayMgr->CloseAllPopups();
        }
    });

    const we::UI::Rect root = overlay->GetGeometry();
    const float width = std::min(root.width * 0.45f, 520.0f);
    const float height = std::min(root.height * 0.55f, 640.0f);
    const we::UI::Point position{
        root.x + (root.width - width) * 0.5f,
        root.y + (root.height - height) * 0.35f
    };
    overlay->ShowPopup(host, position);
}

void EditorPanelController::DockPanel(EditorPanelId id, EditorDockZone zone) {
    auto it = m_Panels.find(id);
    if (it == m_Panels.end() || !it->second.panel) {
        return;
    }

    auto& entry = it->second;
    if (entry.zone != zone) {
        if (auto oldDock = GetDock(entry.zone)) {
            oldDock->RemovePanel(entry.panel);
        }
        entry.zone = zone;
    }

    entry.floating = false;
    entry.visible = true;
    EnsurePanelInDock(entry);
    FocusPanel(id);

    if (m_OnVisibilityChanged) {
        m_OnVisibilityChanged();
    }
}

std::string EditorPanelController::GetMenuLabel(EditorPanelId id) const {
    auto it = m_Panels.find(id);
    return it != m_Panels.end() ? it->second.menuLabel : "";
}

std::shared_ptr<we::UI::Panel> EditorPanelController::GetPanel(EditorPanelId id) const {
    auto it = m_Panels.find(id);
    return it != m_Panels.end() ? it->second.panel : nullptr;
}

void EditorPanelController::SetOnPanelVisibilityChanged(std::function<void()> callback) {
    m_OnVisibilityChanged = std::move(callback);
}

void EditorPanelController::EnsurePanelInDock(const PanelEntry& entry) {
    if (!entry.panel || entry.floating || entry.zone == EditorDockZone::Floating) {
        return;
    }

    auto dock = GetDock(entry.zone);
    if (!dock) {
        HE_ERROR(std::string("[EditorPanel] Dock zone not registered for panel '") + entry.menuLabel + "'.");
        return;
    }

    entry.panel->SetVisible(true);
    if (!dock->ContainsPanel(entry.panel)) {
        dock->AddPanel(entry.panel);
    }
    dock->FocusPanel(entry.panel);
}

std::shared_ptr<we::UI::DockContainer> EditorPanelController::GetDock(EditorDockZone zone) const {
    auto it = m_DockZones.find(zone);
    if (it == m_DockZones.end()) {
        return nullptr;
    }
    return it->second.lock();
}

} // namespace we::programs::editor
