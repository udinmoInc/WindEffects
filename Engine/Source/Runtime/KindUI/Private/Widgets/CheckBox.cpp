#include "KindUI/Widgets/CheckBox.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

namespace we::runtime::kindui {

CheckBox::CheckBox(const std::string& label, bool initialState)
    : m_Label(label)
    , m_Checked(initialState)
    , m_Style(TextStyle::Body())
{
}

Size CheckBox::Measure(const Size& availableSize) {
    (void)availableSize;
    const ResolvedStyle role = ThemeManager::Get().Resolve(StyleRole::Checkbox);
    m_BoxSize = role.height > 0.0f ? role.height : 14.0f;
    const float textWidth = static_cast<float>(m_Label.length()) * (m_Style.size * 0.55f);
    const float gap = ResolveMetric(MetricToken::Space2);
    m_DesiredSize = Size{ m_BoxSize + gap + textWidth, std::max(m_BoxSize, m_Style.size + 4.0f) };
    return m_DesiredSize;
}

void CheckBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void CheckBox::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());

    const Rect boxRect{
        m_Geometry.x,
        m_Geometry.y + (m_Geometry.height - m_BoxSize) * 0.5f,
        m_BoxSize,
        m_BoxSize
    };
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, m_Checked, m_Focused, false };
    ControlChrome::PaintCheckbox(context, boxRect, m_Checked, state);

    const float gap = ResolveMetric(MetricToken::Space2);
    context.DrawText(
        m_Label,
        Point{ m_Geometry.x + m_BoxSize + gap, m_Geometry.y + (m_Geometry.height - m_Style.size) * 0.5f },
        m_Style.color,
        m_Style.size,
        m_Style.bold,
        m_Style.italic);
}

void CheckBox::OnMouseMove(const MouseEvent& event) {
    if (!m_Visible) {
        return;
    }
    m_Hovered = m_Geometry.Contains(event.position);
}

void CheckBox::OnMouseDown(const MouseEvent& event) {
    if (!m_Visible) {
        return;
    }
    if (m_Hovered && event.button == MouseButton::Left) {
        m_Checked = !m_Checked;
        if (m_OnChanged) {
            m_OnChanged(m_Checked);
        }
    }
}

} // namespace we::runtime::kindui
