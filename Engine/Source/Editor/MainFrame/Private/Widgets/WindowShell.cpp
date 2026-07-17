#include "Widgets/WindowShell.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeToken.h"

namespace we::runtime::kindui {

WindowShell::WindowShell() = default;

void WindowShell::SetContent(const std::shared_ptr<Widget>& content) {
    if (m_Content) {
        RemoveChild(m_Content);
    }
    m_Content = content;
    if (m_Content) {
        AddChild(m_Content);
    }
}

Size WindowShell::Measure(const Size& availableSize) {
    if (m_Content) {
        m_DesiredSize = m_Content->Measure(availableSize);
    } else {
        m_DesiredSize = availableSize;
    }
    return m_DesiredSize;
}

void WindowShell::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_Content) {
        m_Content->Arrange(allottedRect);
    }
}

void WindowShell::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::WindowBackground));

    if (m_Content) {
        m_Content->Paint(context);
    }

    // Square frame — bottom edge stays flush with the system edge (no rounded cutout).
    const Color border = ThemeColor(ThemeToken::BorderDefault);
    const float x = m_Geometry.x;
    const float y = m_Geometry.y;
    const float w = m_Geometry.width;
    const float h = m_Geometry.height;
    const float bottom = y + h - 1.0f;
    const float right = x + w - 1.0f;

    context.DrawLine(Point{ x, y }, Point{ right, y }, border, 1.0f);
    context.DrawLine(Point{ x, y }, Point{ x, bottom }, border, 1.0f);
    context.DrawLine(Point{ right, y }, Point{ right, bottom }, border, 1.0f);
    context.DrawLine(Point{ x, bottom }, Point{ right, bottom }, border, 1.0f);
}

} // namespace we::runtime::kindui
