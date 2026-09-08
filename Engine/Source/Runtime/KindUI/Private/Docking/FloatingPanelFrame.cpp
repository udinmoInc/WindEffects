#include "KindUI/Docking/FloatingPanelFrame.h"

#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui::docking {
namespace {

using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::SurfaceRole;

constexpr float kMinFloatWidth = 280.0f;
constexpr float kMinFloatHeight = 200.0f;
constexpr float kMinTitleOnlyHeight = 28.0f;

[[nodiscard]] bool HasEdge(FloatingPanelFrame::ResizeEdge edge, FloatingPanelFrame::ResizeEdge flag) {
    return (static_cast<uint8_t>(edge) & static_cast<uint8_t>(flag)) != 0;
}

} // namespace

FloatingPanelFrame::FloatingPanelFrame() = default;

float FloatingPanelFrame::WindowControlsWidth() const {
    const float scale = ::we::runtime::kindui::panels::PanelChrome::UiScale();
    const float controlW = ::we::runtime::kindui::ResolveMetric(MetricToken::WindowControlWidth) * scale;
    return controlW * 3.0f;
}

void FloatingPanelFrame::SyncDockTrailingReserve() {
    if (!m_Dock) {
        return;
    }
    m_Dock->SetTrailingReservedWidth(WindowControlsWidth());
    m_Dock->SetLeadingReservedWidth(LeadingLogoWidth());
    m_Dock->SetShowOptionsMenu(false);
}

float FloatingPanelFrame::LeadingLogoWidth() const {
    const float scale = ::we::runtime::kindui::panels::PanelChrome::UiScale();
    const float pad = ::we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale;
    const float icon = 16.0f * scale;
    // Logo + pads only on floating windows — docked panels keep flush tabs.
    return pad + icon + pad;
}

void FloatingPanelFrame::SetDock(std::shared_ptr<DockContainer> dock) {
    if (m_Dock) {
        RemoveChild(m_Dock);
    }
    m_Dock = std::move(dock);
    if (m_Dock) {
        SyncDockTrailingReserve();
        AddChild(m_Dock);
    }
}

std::shared_ptr<DockContainer> FloatingPanelFrame::TakeDock() {
    auto dock = m_Dock;
    if (m_Dock) {
        RemoveChild(m_Dock);
        m_Dock.reset();
    }
    return dock;
}

void FloatingPanelFrame::SetPanel(std::shared_ptr<::we::runtime::kindui::panels::Panel> panel) {
    if (!panel) {
        return;
    }
    if (!m_Dock) {
        auto dock = std::make_shared<DockContainer>();
        dock->SetHeaderHeightLogical(
            ::we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight));
        SetDock(std::move(dock));
    }
    if (!m_Dock->ContainsPanel(panel)) {
        m_Dock->AddPanel(panel);
    }
    m_Dock->FocusPanel(panel);
}

std::shared_ptr<::we::runtime::kindui::panels::Panel> FloatingPanelFrame::GetActivePanel() const {
    return m_Dock ? m_Dock->GetActivePanel() : nullptr;
}

std::shared_ptr<::we::runtime::kindui::panels::Panel> FloatingPanelFrame::TakePanel(
    const std::shared_ptr<::we::runtime::kindui::panels::Panel>& panel) {
    if (!m_Dock || !panel || !m_Dock->ContainsPanel(panel)) {
        return nullptr;
    }
    m_Dock->RemovePanel(panel);
    return panel;
}

void FloatingPanelFrame::SetMaximized(bool maximized) {
    if (m_Maximized == maximized) {
        return;
    }

    if (maximized) {
        m_RestoreBounds = m_Geometry;
        m_Maximized = true;
        m_Minimized = false;
        if (m_WorkspaceBounds.width > 1.0f && m_WorkspaceBounds.height > 1.0f) {
            const float margin = 8.0f;
            const Rect bounds{
                m_WorkspaceBounds.x + margin,
                m_WorkspaceBounds.y + margin,
                (std::max)(kMinFloatWidth, m_WorkspaceBounds.width - margin * 2.0f),
                (std::max)(kMinFloatHeight, m_WorkspaceBounds.height - margin * 2.0f)
            };
            if (m_OnResize) {
                m_OnResize(bounds);
            }
        }
    } else {
        m_Maximized = false;
        Rect bounds = m_RestoreBounds;
        if (bounds.width < kMinFloatWidth || bounds.height < kMinFloatHeight) {
            bounds = Rect{
                m_Geometry.x,
                m_Geometry.y,
                (std::max)(m_Geometry.width, kMinFloatWidth),
                (std::max)(m_Geometry.height, kMinFloatHeight)
            };
        }
        if (m_OnResize) {
            m_OnResize(bounds);
        }
    }
}

float FloatingPanelFrame::TitleBarHeight() const {
    if (m_Dock && m_Dock->GetHeaderHeightDevice() > 1.0f) {
        return m_Dock->GetHeaderHeightDevice();
    }
    return ::we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

float FloatingPanelFrame::ResizeBorder() const {
    return 6.0f * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

void FloatingPanelFrame::RelayoutChrome() {
    SyncDockTrailingReserve();
    const float titleH = TitleBarHeight();
    const float scale = ::we::runtime::kindui::panels::PanelChrome::UiScale();
    const float controlW = ::we::runtime::kindui::ResolveMetric(MetricToken::WindowControlWidth) * scale;
    const float controlH = titleH;
    const float pad = ::we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale;
    const float icon = 16.0f * scale;

    m_TitleBarRect = Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, titleH };
    m_ContentRect = m_Geometry;
    m_LogoRect = Rect{
        m_Geometry.x + pad,
        m_Geometry.y + (titleH - icon) * 0.5f,
        icon,
        icon
    };

    const float right = m_Geometry.x + m_Geometry.width;
    m_CloseRect = Rect{ right - controlW, m_Geometry.y, controlW, controlH };
    m_MaximizeRect = Rect{ m_CloseRect.x - controlW, m_Geometry.y, controlW, controlH };
    m_MinimizeRect = Rect{ m_MaximizeRect.x - controlW, m_Geometry.y, controlW, controlH };
}

FloatingPanelFrame::ResizeEdge FloatingPanelFrame::HitResizeEdge(const Point& pos) const {
    if (m_Maximized || m_Minimized) {
        return ResizeEdge::None;
    }

    const float b = ResizeBorder();
    const Rect g = m_Geometry;
    if (!(pos.x >= g.x - b && pos.x <= g.x + g.width + b
            && pos.y >= g.y - b && pos.y <= g.y + g.height + b)) {
        return ResizeEdge::None;
    }

    uint8_t flags = 0;
    if (pos.x <= g.x + b) {
        flags |= static_cast<uint8_t>(ResizeEdge::Left);
    }
    if (pos.x >= g.x + g.width - b) {
        flags |= static_cast<uint8_t>(ResizeEdge::Right);
    }
    if (pos.y <= g.y + b) {
        flags |= static_cast<uint8_t>(ResizeEdge::Top);
    }
    if (pos.y >= g.y + g.height - b) {
        flags |= static_cast<uint8_t>(ResizeEdge::Bottom);
    }

    if (flags == static_cast<uint8_t>(ResizeEdge::Top) && m_TitleBarRect.Contains(pos)
        && pos.y > g.y + b * 0.5f) {
        return ResizeEdge::None;
    }

    return static_cast<ResizeEdge>(flags);
}

void FloatingPanelFrame::EmitBounds() {
    if (m_OnResize) {
        m_OnResize(m_Geometry);
    }
}

void FloatingPanelFrame::ApplyResizeDelta(const Point& delta) {
    Rect g = m_Geometry;
    if (HasEdge(m_ActiveEdge, ResizeEdge::Left)) {
        const float maxShrink = g.width - kMinFloatWidth;
        const float dx = std::clamp(delta.x, -1.0e6f, maxShrink);
        g.x += dx;
        g.width -= dx;
    }
    if (HasEdge(m_ActiveEdge, ResizeEdge::Right)) {
        g.width = (std::max)(kMinFloatWidth, g.width + delta.x);
    }
    if (HasEdge(m_ActiveEdge, ResizeEdge::Top)) {
        const float minH = m_Minimized ? kMinTitleOnlyHeight : kMinFloatHeight;
        const float maxShrink = g.height - minH;
        const float dy = std::clamp(delta.y, -1.0e6f, maxShrink);
        g.y += dy;
        g.height -= dy;
    }
    if (HasEdge(m_ActiveEdge, ResizeEdge::Bottom)) {
        const float minH = m_Minimized ? kMinTitleOnlyHeight : kMinFloatHeight;
        g.height = (std::max)(minH, g.height + delta.y);
    }
    m_Geometry = g;
    EmitBounds();
}

Size FloatingPanelFrame::Measure(const Size& availableSize) {
    Size body{ 0.0f, 0.0f };
    if (m_Dock && !m_Minimized) {
        body = m_Dock->Measure(availableSize);
    } else {
        body = Size{ kMinFloatWidth, TitleBarHeight() };
    }
    m_DesiredSize = ClampDesiredSize(Size{
        availableSize.width < 1.0e8f ? availableSize.width
            : (std::max)(kMinFloatWidth, body.width),
        availableSize.height < 1.0e8f ? availableSize.height
            : (std::max)(TitleBarHeight(), body.height)
    });
    return m_DesiredSize;
}

void FloatingPanelFrame::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    RelayoutChrome();
    if (m_Dock) {
        if (m_Minimized) {
            m_Dock->SetVisible(false);
            m_Dock->Arrange(Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, TitleBarHeight() });
        } else {
            m_Dock->SetVisible(true);
            m_Dock->Arrange(m_Geometry);
        }
    }
}

void FloatingPanelFrame::Paint(::we::runtime::kindui::PaintContext& context) {
    RelayoutChrome();

    context.PushSurfaceOwner("FloatingPanelFrame", SurfaceRole::Window);
    context.DrawSurface(m_Geometry, SurfaceRole::Window, 0.0f, "FloatingWindow");
    context.DrawSurface(m_TitleBarRect, SurfaceRole::DockChrome, 0.0f, "FloatingTitleBar");
    context.DrawSurfaceOutline(m_Geometry, SurfaceRole::Border, 1.0f, 0.0f, "FloatingBorder");

    // Left-aligned window logo beside the tab strip.
    if (m_LogoRect.width > 0.5f && m_LogoRect.height > 0.5f) {
        context.DrawWindIcon(::we::runtime::kindui::WindIcons::Window16, m_LogoRect);
    }

    if (m_Dock && !m_Minimized) {
        m_Dock->Paint(context);
    }

    auto paintControl = [&](const Rect& rect, auto icon, int controlIndex, bool isClose) {
        const bool hovered = m_HoveredControl == controlIndex;
        if (hovered) {
            const Color hover = isClose
                ? ThemeColor(ColorToken::CloseButtonHover)
                : ThemeColor(ColorToken::HoverBackground);
            context.DrawRect(rect, hover);
        }
        ::we::runtime::kindui::panels::PanelChrome::PaintHeaderIconButton(
            context, rect, icon, hovered, false, true);
    };

    paintControl(m_MinimizeRect, ::we::runtime::kindui::WindIcons::Minus16, 0, false);
    paintControl(
        m_MaximizeRect,
        m_Maximized ? ::we::runtime::kindui::WindIcons::Copy16 : ::we::runtime::kindui::WindIcons::Square16,
        1,
        false);
    paintControl(m_CloseRect, ::we::runtime::kindui::WindIcons::X16, 2, true);
}

void FloatingPanelFrame::OnMouseDown(const ::we::runtime::kindui::MouseEvent& event) {
    m_HoveredControl = -1;
    if (m_CloseRect.Contains(event.position)) {
        m_Dragging = false;
        m_Resizing = false;
        if (m_OnClose) {
            m_OnClose();
        }
        return;
    }
    if (m_MaximizeRect.Contains(event.position)) {
        m_Dragging = false;
        m_Resizing = false;
        SetMaximized(!m_Maximized);
        return;
    }
    if (m_MinimizeRect.Contains(event.position)) {
        m_Dragging = false;
        m_Resizing = false;
        if (m_Minimized) {
            m_Minimized = false;
            Rect bounds = m_RestoreBounds;
            if (bounds.height < kMinFloatHeight) {
                bounds = Rect{
                    m_Geometry.x,
                    m_Geometry.y,
                    (std::max)(m_Geometry.width, kMinFloatWidth),
                    kMinFloatHeight
                };
            }
            bounds.x = m_Geometry.x;
            bounds.y = m_Geometry.y;
            bounds.width = (std::max)(bounds.width, m_Geometry.width);
            if (m_OnResize) {
                m_OnResize(bounds);
            }
        } else {
            if (!m_Maximized) {
                m_RestoreBounds = m_Geometry;
            }
            m_Minimized = true;
            m_Maximized = false;
            const Rect bounds{ m_Geometry.x, m_Geometry.y, m_Geometry.width, TitleBarHeight() };
            if (m_OnResize) {
                m_OnResize(bounds);
            }
        }
        return;
    }

    const ResizeEdge edge = HitResizeEdge(event.position);
    if (edge != ResizeEdge::None) {
        m_Resizing = true;
        m_Dragging = false;
        m_ActiveEdge = edge;
        m_DragLast = event.position;
        return;
    }

    // Empty tab-strip / header drag area moves the floating window.
    if (m_TitleBarRect.Contains(event.position)
        && (!m_Dock || !m_Dock->IsTabStripInteractiveHit(event.position))) {
        if (m_Maximized) {
            SetMaximized(false);
        }
        m_Dragging = true;
        m_Resizing = false;
        m_DragLast = event.position;
        return;
    }

    if (m_Dock && !m_Minimized) {
        m_Dock->OnMouseDown(event);
    }
}

void FloatingPanelFrame::OnHoverLost() {
    m_HoveredControl = -1;
}

void FloatingPanelFrame::OnMouseMove(const ::we::runtime::kindui::MouseEvent& event) {
    if (m_Resizing) {
        const Point delta{
            event.position.x - m_DragLast.x,
            event.position.y - m_DragLast.y
        };
        m_DragLast = event.position;
        if (std::abs(delta.x) > 0.01f || std::abs(delta.y) > 0.01f) {
            ApplyResizeDelta(delta);
        }
        return;
    }

    if (m_Dragging && m_OnMove) {
        const Point delta{
            event.position.x - m_DragLast.x,
            event.position.y - m_DragLast.y
        };
        m_DragLast = event.position;
        if (std::abs(delta.x) > 0.01f || std::abs(delta.y) > 0.01f) {
            m_OnMove(delta);
        }
        return;
    }

    if (m_CloseRect.Contains(event.position)) {
        m_HoveredControl = 2;
    } else if (m_MaximizeRect.Contains(event.position)) {
        m_HoveredControl = 1;
    } else if (m_MinimizeRect.Contains(event.position)) {
        m_HoveredControl = 0;
    } else {
        m_HoveredControl = -1;
    }

    if (m_Dock && !m_Minimized) {
        m_Dock->OnMouseMove(event);
    }
}

void FloatingPanelFrame::OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) {
    m_Dragging = false;
    m_Resizing = false;
    m_ActiveEdge = ResizeEdge::None;
    if (m_Dock && !m_Minimized) {
        m_Dock->OnMouseUp(event);
    }
}

std::shared_ptr<::we::runtime::kindui::Widget> FloatingPanelFrame::HitTestPoint(
    const Point& pos,
    const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }

    const float b = ResizeBorder();
    const Rect hitBounds{
        m_Geometry.x - b,
        m_Geometry.y - b,
        m_Geometry.width + b * 2.0f,
        m_Geometry.height + b * 2.0f
    };
    if ((clip != nullptr && !clip->Contains(pos)) || !hitBounds.Contains(pos)) {
        return nullptr;
    }

    if (HitResizeEdge(pos) != ResizeEdge::None
        || m_MinimizeRect.Contains(pos)
        || m_MaximizeRect.Contains(pos)
        || m_CloseRect.Contains(pos)) {
        return shared_from_this();
    }

    if (m_TitleBarRect.Contains(pos)
        && (!m_Dock || !m_Dock->IsTabStripInteractiveHit(pos))) {
        return shared_from_this();
    }

    if (m_Dock && !m_Minimized) {
        if (auto hit = m_Dock->HitTestPoint(pos, clip)) {
            return hit;
        }
    }

    if (m_Geometry.Contains(pos)) {
        return shared_from_this();
    }
    return nullptr;
}

} // namespace we::runtime::kindui::docking
 
