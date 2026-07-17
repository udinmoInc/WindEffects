#include "WindEffects/Runtime/UI/Widgets/Button.h"
#include "WindEffects/Runtime/UI/Core/ControlChrome.h"
#include "WindEffects/Runtime/UI/Core/PaintContext.h"
#include "WindEffects/Runtime/UI/Core/TextMetrics.h"
#include "WindEffects/Runtime/UI/Theming/ThemeManager.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"
#include "WindEffects/Runtime/UI/Core/Animator.h"

namespace WindEffects::Editor::UI {

Button::Button(const std::string& labelText, std::function<void()> onClicked)
    : m_Text(labelText)
    , m_OnClicked(onClicked) {
    SetFocusable(false);
}

Size Button::Measure(const Size& availableSize) {
    (void)availableSize;
    const ResolvedStyle role = ThemeManager::Get().Resolve(StyleRole::ButtonSecondary);
    const float textWidth = TextMetrics::EstimateWidth(m_Text, role.fontSize);
    const float pad = ResolveThemeMetric(ThemeToken::ButtonPaddingHorizontal);
    m_DesiredSize = Size{
        textWidth + pad * 2.0f,
        role.height > 0.0f ? role.height : ResolveThemeMetric(ThemeToken::ButtonHeight)
    };
    return m_DesiredSize;
}

void Button::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Button::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, ControlChrome::PressDamping());

    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::ButtonSecondary);
    ControlChrome::InteractionState state{ m_HoverAnim, m_PressAnim, false, m_Focused, false };
    ControlChrome::PaintFilledButton(context, m_Geometry, style, state);
    ControlChrome::PaintCenteredLabel(
        context,
        m_Geometry,
        m_Text,
        style.foreground,
        style.fontSize,
        style.bold);
}

void Button::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        SetPressed(true);
    }
}

void Button::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        SetPressed(false);
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

} // namespace WindEffects::Editor::UI