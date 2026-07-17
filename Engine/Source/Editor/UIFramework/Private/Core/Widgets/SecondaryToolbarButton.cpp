#include "Core/Widgets/SecondaryToolbarButton.h"
#include "Core/PaintContext.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Rendering/IconMetrics.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

SecondaryToolbarButton::SecondaryToolbarButton(const std::string& label, const std::string& iconName)
    : m_Label(label)
    , m_IconName(iconName)
{}

Size SecondaryToolbarButton::Measure(const Size& availableSize) {
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

void SecondaryToolbarButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(ThemeMetric(ThemeToken::ButtonHeight), allottedRect.height);
    const float w = m_DesiredSize.width > 0.0f
        ? std::min(m_DesiredSize.width, allottedRect.width)
        : allottedRect.width;
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        w,
        h
    };
}

void SecondaryToolbarButton::Paint(PaintContext& context) {
    const auto baseStyle = ResolveStyle(StyleRole::ButtonSecondary);
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
        bgColor = Color::Lerp(bgColor, hoverStyle.background, m_HoverAnim);
        textColor = Color::Lerp(textColor, hoverStyle.foreground, m_HoverAnim);
        borderColor = Color::Lerp(borderColor, hoverStyle.border, m_HoverAnim);
    }
    if (m_Enabled && m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, pressStyle.background, m_PressAnim);
    }

    Rect buttonRect = m_Geometry;
    buttonRect.y += m_PressOffset;

    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(buttonRect, bgColor, radius);
    }

    if (m_Enabled && borderColor.a > 0.01f) {
        context.DrawRoundedRectOutline(buttonRect, borderColor, 1.0f, radius);
    }

    const float hPad = ThemeMetric(ThemeToken::ButtonPaddingHorizontal);
    float x = buttonRect.x + hPad;
    const float textSize = baseStyle.fontSize;
    const float textY = buttonRect.y + (buttonRect.height - textSize) * 0.5f;

    if (!m_IconName.empty()) {
        const float iconSize = static_cast<float>(IconMetrics::StandardGlyphTierPx());
        Rect iconBand{ x, buttonRect.y, iconSize, buttonRect.height };
        IconPainter::DrawIcon(context, m_IconName, IconMetrics::PlaceGlyphCentered(iconBand, iconSize), textColor);
        x += iconSize + ThemeMetric(ThemeToken::Space2);
    }

    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, false);
}

void SecondaryToolbarButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Enabled) m_Pressed = true;
}

void SecondaryToolbarButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void SecondaryToolbarButton::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

} // namespace WindEffects::Editor::UI
