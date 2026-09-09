// ==============================================================================
// WindEffects — KindUI — FloatingPanelFrame
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Docking/DockContainer.h"
#include "KindUI/Panel/Panel.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace we::runtime::kindui::docking {

/// Opaque floating window chrome hosting a shared DockContainer (multi-panel tabs).
/// Single tab strip + window controls — no nested title bar / duplicate chrome.
class KINDUI_API FloatingPanelFrame final : public ::we::runtime::kindui::Widget {
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

    void SetDock(std::shared_ptr<DockContainer> dock);
    [[nodiscard]] std::shared_ptr<DockContainer> GetDock() const { return m_Dock; }
    [[nodiscard]] std::shared_ptr<DockContainer> TakeDock();

    /// Convenience: ensure a dock exists and add/focus a single panel.
    void SetPanel(std::shared_ptr<::we::runtime::kindui::panels::Panel> panel);
    [[nodiscard]] std::shared_ptr<::we::runtime::kindui::panels::Panel> GetActivePanel() const;
    [[nodiscard]] std::shared_ptr<::we::runtime::kindui::panels::Panel> TakePanel(
        const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel);

    void SetOnMove(std::function<void(const ::we::runtime::kindui::Point& delta)> handler) {
        m_OnMove = std::move(handler);
    }
    void SetOnResize(std::function<void(const ::we::runtime::kindui::Rect& bounds)> handler) {
        m_OnResize = std::move(handler);
    }
    void SetOnClose(std::function<void()> handler) { m_OnClose = std::move(handler); }
    void SetWorkspaceBounds(const ::we::runtime::kindui::Rect& bounds) { m_WorkspaceBounds = bounds; }
    void SetMaximized(bool maximized);
    [[nodiscard]] bool IsMaximized() const { return m_Maximized; }
    [[nodiscard]] float WindowControlsWidth() const;
    [[nodiscard]] float LeadingLogoWidth() const;

    ::we::runtime::kindui::Size Measure(const ::we::runtime::kindui::Size& availableSize) override;
    void Arrange(const ::we::runtime::kindui::Rect& allottedRect) override;
    void Paint(::we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const ::we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const ::we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) override;
    void OnHoverLost() override;
    [[nodiscard]] std::shared_ptr<Widget> HitTestPoint(
        const ::we::runtime::kindui::Point& pos,
        const ::we::runtime::kindui::Rect* clip = nullptr) override;
    [[nodiscard]] bool IsInteractiveContainer() const override { return true; }

private:
    void RelayoutChrome();
    void SyncDockTrailingReserve();
    [[nodiscard]] float TitleBarHeight() const;
    [[nodiscard]] float ResizeBorder() const;
    [[nodiscard]] ResizeEdge HitResizeEdge(const ::we::runtime::kindui::Point& pos) const;
    void ApplyResizeDelta(const ::we::runtime::kindui::Point& delta);
    void EmitBounds();

    std::shared_ptr<DockContainer> m_Dock;
    std::function<void(const ::we::runtime::kindui::Point& delta)> m_OnMove;
    std::function<void(const ::we::runtime::kindui::Rect& bounds)> m_OnResize;
    std::function<void()> m_OnClose;

    ::we::runtime::kindui::Rect m_TitleBarRect{};
    ::we::runtime::kindui::Rect m_ContentRect{};
    ::we::runtime::kindui::Rect m_LogoRect{};
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
    int m_HoveredControl = -1;
    ::we::runtime::kindui::Point m_DragLast{};
};

} // namespace we::runtime::kindui::docking
