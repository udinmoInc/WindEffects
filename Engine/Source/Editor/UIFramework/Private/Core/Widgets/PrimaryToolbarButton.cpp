#include "Core/Widgets/PrimaryToolbarButton.h"
#include "Core/PaintContext.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

PrimaryToolbarButton::PrimaryToolbarButton(const std::string& label, const std::string& iconName)
    : m_Label(label)
    , m_IconName(iconName)
{}

Size PrimaryToolbarButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float hPad = ThemeMetric(ThemeToken::ButtonPaddingHorizontal);
    const float iconSize = ThemeMetric(ThemeToken::IconSizeToolbar);
    const float iconGap = ThemeMetric(ThemeToken::Space2) - 2.0f;
    float width = hPad * 2.0f;
    if (!m_IconName.empty()) width += iconSize + iconGap;
    width += static_cast<float>(m_Label.size()) * ThemeMetric(ThemeToken::TextSizeToolbar) * 0.55f;
    m_DesiredSize = Size{ width, ThemeMetric(ThemeToken::ButtonHeight) };
    return m_DesiredSize;
}

void PrimaryToolbarButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(ThemeMetric(ThemeToken::ButtonHeight), allottedRect.height);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

void PrimaryToolbarButton::Paint(PaintContext& context) {
    const auto baseStyle = ResolveStyle(StyleRole::ButtonPrimary);
    const auto hoverStyle = ResolveStyle(StyleRole::ButtonHover);
    const auto pressStyle = ResolveStyle(StyleRole::ButtonActive);

    const float hoverDamping = ThemeMetric(ThemeToken::HoverAnimationDamping);
    const float pressDamping = ThemeMetric(ThemeToken::PressAnimationDamping);
    const float pressOffsetTarget = ThemeMetric(ThemeToken::PressOffset);

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, hoverDamping);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed && m_Enabled ? 1.0f : 0.0f, pressDamping);

    const float targetOffset = m_Pressed && m_Enabled ? pressOffsetTarget : 0.0f;
    m_PressOffset = Animator::Damp(m_PressOffset, targetOffset, pressDamping);

    const float radius = baseStyle.cornerRadius;

    Color bgColor = m_Enabled ? baseStyle.background : ThemeColor(ThemeToken::DisabledBackground);
    Color borderColor = m_Enabled ? baseStyle.border : ThemeColor(ThemeToken::BorderDark);
    Color textColor = m_Enabled ? baseStyle.foreground : ThemeColor(ThemeToken::TextDisabled);

    if (m_Enabled && m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, ThemeColor(ThemeToken::ButtonPrimaryHover), m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, ThemeColor(ThemeToken::ButtonPrimaryPressed), m_PressAnim);
    }

    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    context.DrawRoundedRect(buttonRect, bgColor, radius);

    if (m_Enabled) {
        context.DrawRoundedRectOutline(
            Rect{ buttonRect.x + 0.5f, buttonRect.y + 0.5f, buttonRect.width - 1.0f, buttonRect.height - 1.0f },
            ThemeColor(ThemeToken::BorderLight), baseStyle.borderWidth * 0.5f, radius
        );
    }

    context.DrawRoundedRectOutline(buttonRect, borderColor, baseStyle.borderWidth * 0.5f, radius);

    const float hPad = ThemeMetric(ThemeToken::ButtonPaddingHorizontal);
    float x = buttonRect.x + hPad;
    const float textSize = baseStyle.fontSize;
    const float textY = buttonRect.y + (buttonRect.height - textSize) * 0.5f;

    if (!m_IconName.empty()) {
        const float iconSize = baseStyle.iconSize;
        const float iconY = buttonRect.y + (buttonRect.height - iconSize) * 0.5f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ x, iconY, iconSize, iconSize }, textColor);
        x += iconSize + ThemeMetric(ThemeToken::Space2) - 2.0f;
    }

    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, false);
}

void PrimaryToolbarButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Enabled) m_Pressed = true;
}

void PrimaryToolbarButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void PrimaryToolbarButton::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

} // namespace WindEffects::Editor::UI
