#include "WindEffects/Editor/UI/Widgets/FloatingPanelFrame.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cmath>

namespace we::editor::docking {
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

void FloatingPanelFrame::SetPanel(std::shared_ptr<::we::editor::panels::Panel> panel) {
    if (m_Panel) {
        RemoveChild(m_Panel);
    }
    m_Panel = std::move(panel);
    if (m_Panel) {
        m_Panel->SetHeaderHeight(0.0f);
        m_Panel->SetTransparentBackground(false);
        AddChild(m_Panel);
    }
}

std::shared_ptr<::we::editor::panels::Panel> FloatingPanelFrame::TakePanel() {
    auto panel = m_Panel;
    if (m_Panel) {
        RemoveChild(m_Panel);
        m_Panel.reset();
    }
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
    return ::we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight)
        * ::we::editor::panels::PanelChrome::UiScale();
}

float FloatingPanelFrame::ResizeBorder() const {
    return 6.0f * ::we::editor::panels::PanelChrome::UiScale();
}

void FloatingPanelFrame::RelayoutChrome() {
    const float titleH = TitleBarHeight();
    const float scale = ::we::editor::panels::PanelChrome::UiScale();
    const float controlW = ::we::runtime::kindui::ResolveMetric(MetricToken::WindowControlWidth) * scale;
    const float controlH = titleH;

    m_TitleBarRect = Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, titleH };
    m_ContentRect = Rect{
        m_Geometry.x,
        m_Geometry.y + titleH,
        m_Geometry.width,
        (std::max)(0.0f, m_Geometry.height - titleH)
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
    if (!g.Contains(pos)
        && !(pos.x >= g.x - b && pos.x <= g.x + g.width + b
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

    // Prefer title-bar drag over top-edge resize unless near the physical edge.
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
    const float titleH = TitleBarHeight();
    Size bodyAvail = availableSize;
    if (bodyAvail.height < 1.0e8f) {
        bodyAvail.height = (std::max)(0.0f, bodyAvail.height - titleH);
    }
    Size body{ 0.0f, 0.0f };
    if (m_Panel && !m_Minimized) {
        body = m_Panel->Measure(bodyAvail);
    }
    m_DesiredSize = ClampDesiredSize(Size{
        availableSize.width < 1.0e8f ? availableSize.width
            : (std::max)(kMinFloatWidth, body.width),
        availableSize.height < 1.0e8f ? availableSize.height
            : (titleH + (m_Minimized ? 0.0f : body.height))
    });
    return m_DesiredSize;
}

void FloatingPanelFrame::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    RelayoutChrome();
    if (m_Panel) {
        if (m_Minimized || m_ContentRect.height < 1.0f) {
            m_Panel->SetVisible(false);
            m_Panel->Arrange(Rect{ m_ContentRect.x, m_ContentRect.y, m_ContentRect.width, 0.0f });
        } else {
            m_Panel->SetVisible(true);
            m_Panel->Arrange(m_ContentRect);
        }
    }
}

void FloatingPanelFrame::Paint(::we::runtime::kindui::PaintContext& context) {
    RelayoutChrome();

    // Opaque window frame — GraphiteDark Window / Panel surfaces (no transparency).
    context.PushSurfaceOwner("FloatingPanelFrame", SurfaceRole::Window);
    context.DrawSurface(m_Geometry, SurfaceRole::Window, 0.0f, "FloatingWindow");
    context.DrawSurface(m_TitleBarRect, SurfaceRole::Window, 0.0f, "FloatingTitleBar");

    if (!m_Minimized && m_ContentRect.height > 0.5f) {
        context.DrawSurface(m_ContentRect, SurfaceRole::Panel, 0.0f, "FloatingContent");
    }

    context.DrawSurfaceOutline(m_Geometry, SurfaceRole::Border, 1.0f, 0.0f, "FloatingBorder");

    // Title + icon
    if (m_Panel) {
        const float scale = ::we::editor::panels::PanelChrome::UiScale();
        const float pad = ::we::runtime::kindui::ResolveMetric(MetricToken::Space2) * scale;
        const float iconSize = 16.0f * scale;
        float textX = m_TitleBarRect.x + pad;

        if (m_Panel->GetTabIcon().IsValid()) {
            const Rect iconRect{
                textX,
                m_TitleBarRect.y + (m_TitleBarRect.height - iconSize) * 0.5f,
                iconSize,
                iconSize
            };
            context.DrawWindIcon(m_Panel->GetTabIcon(), iconRect);
            textX = iconRect.x + iconRect.width + pad;
        }

        const float fontSize = 12.0f * scale;
        const float textMaxW = (std::max)(
            0.0f,
            m_MinimizeRect.x - textX - pad);
        if (textMaxW > 8.0f) {
            context.DrawText(
                m_Panel->GetTitle(),
                Point{
                    textX,
                    m_TitleBarRect.y + (m_TitleBarRect.height - fontSize) * 0.5f
                },
                ThemeColor(ColorToken::TextPrimary),
                fontSize,
                false,
                false);
        }
    }

    auto paintControl = [&](const Rect& rect, auto icon, int controlIndex, bool isClose) {
        const bool hovered = m_HoveredControl == controlIndex;
        if (hovered) {
            const Color hover = isClose
                ? ThemeColor(ColorToken::CloseButtonHover)
                : ThemeColor(ColorToken::HoverBackground);
            context.DrawRect(rect, hover);
        }
        ::we::editor::panels::PanelChrome::PaintHeaderIconButton(
            context, rect, icon, hovered, false, true);
    };

    paintControl(m_MinimizeRect, ::we::runtime::kindui::WindIcons::Minus16, 0, false);
    paintControl(
        m_MaximizeRect,
        m_Maximized ? ::we::runtime::kindui::WindIcons::Copy16 : ::we::runtime::kindui::WindIcons::Square16,
        1,
        false);
    paintControl(m_CloseRect, ::we::runtime::kindui::WindIcons::X16, 2, true);

    if (m_Panel && !m_Minimized) {
        m_Panel->Paint(context);
    }
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
            const Rect bounds{
                m_Geometry.x,
                m_Geometry.y,
                m_Geometry.width,
                TitleBarHeight()
            };
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

    if (m_TitleBarRect.Contains(event.position)) {
        if (m_Maximized) {
            // Restore then drag from cursor.
            const Point local{
                event.position.x - m_Geometry.x,
                event.position.y - m_Geometry.y
            };
            SetMaximized(false);
            const float ratio = m_Geometry.width > 1.0f
                ? std::clamp(local.x / (m_RestoreBounds.width > 1.0f ? m_RestoreBounds.width : m_Geometry.width), 0.0f, 1.0f)
                : 0.5f;
            (void)ratio;
        }
        m_Dragging = true;
        m_Resizing = false;
        m_DragLast = event.position;
        return;
    }

    if (m_Panel && !m_Minimized) {
        m_Panel->OnMouseDown(event);
    }
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
        if (m_OnDragOver) {
            m_OnDragOver(event.position);
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

    if (m_Panel && !m_Minimized) {
        m_Panel->OnMouseMove(event);
    }
}

void FloatingPanelFrame::OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) {
    const bool wasDragging = m_Dragging;
    m_Dragging = false;
    m_Resizing = false;
    m_ActiveEdge = ResizeEdge::None;

    if (wasDragging && m_OnDragEnd) {
        m_OnDragEnd(event.position);
    }

    if (m_Panel && !m_Minimized) {
        m_Panel->OnMouseUp(event);
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

    if (HitResizeEdge(pos) != ResizeEdge::None) {
        return shared_from_this();
    }
    if (m_TitleBarRect.Contains(pos)
        || m_MinimizeRect.Contains(pos)
        || m_MaximizeRect.Contains(pos)
        || m_CloseRect.Contains(pos)) {
        return shared_from_this();
    }
    if (m_Panel && !m_Minimized) {
        if (auto hit = m_Panel->HitTestPoint(pos, clip)) {
            return hit;
        }
    }
    if (m_Geometry.Contains(pos)) {
        return shared_from_this();
    }
    return nullptr;
}

} // namespace we::editor::docking
