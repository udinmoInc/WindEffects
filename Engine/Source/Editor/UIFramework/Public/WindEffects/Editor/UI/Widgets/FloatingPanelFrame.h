#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace we::editor::docking {

/// Native-style floating window chrome for an undocked panel (overlay-hosted until
/// multi-swapchain secondary windows exist). Opaque GraphiteDark surface + title bar
/// with min/max/close, edge resize, and dock-drop callbacks.
class UIFRAMEWORK_API FloatingPanelFrame final : public ::we::runtime::kindui::Widget {
public:
    enum class ResizeEdge : uint8_t {
        None = 0,
        Left = 1,
        Right = 2,
        Top = 4,
        Bottom = 8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right,
    };

    FloatingPanelFrame();

    void SetPanel(std::shared_ptr<::we::editor::panels::Panel> panel);
    [[nodiscard]] std::shared_ptr<::we::editor::panels::Panel> GetPanel() const { return m_Panel; }
    [[nodiscard]] std::shared_ptr<::we::editor::panels::Panel> TakePanel();

    void SetOnMove(std::function<void(const ::we::runtime::kindui::Point& delta)> handler) {
        m_OnMove = std::move(handler);
    }
    void SetOnResize(std::function<void(const ::we::runtime::kindui::Rect& bounds)> handler) {
        m_OnResize = std::move(handler);
    }
    void SetOnClose(std::function<void()> handler) { m_OnClose = std::move(handler); }
    void SetOnDockRequest(std::function<void(DockZone zone)> handler) {
        m_OnDockRequest = std::move(handler);
    }
    void SetOnDragOver(std::function<void(const ::we::runtime::kindui::Point& screenPos)> handler) {
        m_OnDragOver = std::move(handler);
    }
    void SetOnDragEnd(std::function<void(const ::we::runtime::kindui::Point& screenPos)> handler) {
        m_OnDragEnd = std::move(handler);
    }
    void SetWorkspaceBounds(const ::we::runtime::kindui::Rect& bounds) { m_WorkspaceBounds = bounds; }
    void SetMaximized(bool maximized);
    [[nodiscard]] bool IsMaximized() const { return m_Maximized; }

    ::we::runtime::kindui::Size Measure(const ::we::runtime::kindui::Size& availableSize) override;
    void Arrange(const ::we::runtime::kindui::Rect& allottedRect) override;
    void Paint(::we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const ::we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const ::we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) override;
    [[nodiscard]] std::shared_ptr<Widget> HitTestPoint(
        const ::we::runtime::kindui::Point& pos,
        const ::we::runtime::kindui::Rect* clip = nullptr) override;
    [[nodiscard]] bool IsInteractiveContainer() const override { return true; }

private:
    void RelayoutChrome();
    [[nodiscard]] float TitleBarHeight() const;
    [[nodiscard]] float ResizeBorder() const;
    [[nodiscard]] ResizeEdge HitResizeEdge(const ::we::runtime::kindui::Point& pos) const;
    void ApplyResizeDelta(const ::we::runtime::kindui::Point& delta);
    void EmitBounds();

    std::shared_ptr<::we::editor::panels::Panel> m_Panel;
    std::function<void(const ::we::runtime::kindui::Point& delta)> m_OnMove;
    std::function<void(const ::we::runtime::kindui::Rect& bounds)> m_OnResize;
    std::function<void()> m_OnClose;
    std::function<void(DockZone zone)> m_OnDockRequest;
    std::function<void(const ::we::runtime::kindui::Point&)> m_OnDragOver;
    std::function<void(const ::we::runtime::kindui::Point&)> m_OnDragEnd;

    ::we::runtime::kindui::Rect m_TitleBarRect{};
    ::we::runtime::kindui::Rect m_ContentRect{};
    ::we::runtime::kindui::Rect m_MinimizeRect{};
    ::we::runtime::kindui::Rect m_MaximizeRect{};
    ::we::runtime::kindui::Rect m_CloseRect{};
    ::we::runtime::kindui::Rect m_WorkspaceBounds{};
    ::we::runtime::kindui::Rect m_RestoreBounds{};

    bool m_Dragging = false;
    bool m_Resizing = false;
    bool m_Maximized = false;
    bool m_Minimized = false;
    ResizeEdge m_ActiveEdge = ResizeEdge::None;
    int m_HoveredControl = -1; // 0=min 1=max 2=close
    ::we::runtime::kindui::Point m_DragLast{};
};

} // namespace we::editor::docking
