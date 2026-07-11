#include "Widgets/CheckBox.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

namespace WindEffects::Editor::UI {

CheckBox::CheckBox(const std::string& label, bool initialState)
    : m_Label(label), m_Checked(initialState), m_Style(TextStyle::Body())
{
}

Size CheckBox::Measure(const Size& availableSize) {
    (void)availableSize;
    float textWidth = static_cast<float>(m_Label.length() * (m_Style.size * 0.6f));
    m_DesiredSize = Size{ m_BoxSize + 8.0f + textWidth, std::max(m_BoxSize, m_Style.size + 4.0f) };
    return m_DesiredSize;
}

void CheckBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void CheckBox::Paint(PaintContext& context) {
    if (!m_Visible) return;

    // Draw checkbox background
    Rect boxRect = { m_Geometry.x, m_Geometry.y + (m_Geometry.height - m_BoxSize) * 0.5f, m_BoxSize, m_BoxSize };
    context.DrawRect(boxRect, m_Hovered ? ResolveThemeColor(ThemeToken::HoverBackground) : ResolveThemeColor(ThemeToken::InputBackground));

    // Draw checkmark
    if (m_Checked) {
        context.DrawRect(Rect{ boxRect.x + 3.0f, boxRect.y + 3.0f, boxRect.width - 6.0f, boxRect.height - 6.0f }, ResolveThemeColor(ThemeToken::TextPrimary));
    }

    // Draw text
    context.DrawText(m_Label, Point{ m_Geometry.x + m_BoxSize + 8.0f, m_Geometry.y + (m_Geometry.height - m_Style.size) * 0.5f - 2.0f }, m_Style.color, m_Style.size, m_Style.bold, m_Style.italic);
}

void CheckBox::OnMouseMove(const MouseEvent& event) {
    if (!m_Visible) return;
    m_Hovered = m_Geometry.Contains(event.position);
}

void CheckBox::OnMouseDown(const MouseEvent& event) {
    if (!m_Visible) return;
    if (m_Hovered && event.button == MouseButton::Left) {
        m_Checked = !m_Checked;
        if (m_OnChanged) {
            m_OnChanged(m_Checked);
        }
    }
}

} // namespace WindEffects::Editor::UI
