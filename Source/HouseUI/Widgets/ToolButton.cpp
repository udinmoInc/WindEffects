#include "ToolButton.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include <algorithm>

namespace HouseEngine::UI {

ToolButton::ToolButton(const std::string& iconName, const std::string& label, std::function<void()> onClicked, const std::string& tooltip)
    : m_IconName(iconName)
    , m_Label(label)
    , m_Tooltip(tooltip)
    , m_OnClicked(onClicked)
    , m_Style(WidgetStyle::ToolButton())
{}

Size ToolButton::Measure(const Size& availableSize) {
    float height = std::min(36.0f, availableSize.height); // AAA standard toolbar button height
    float paddingX = 8.0f; // Tighter padding
    float iconSize = 20.0f; // 20x20 strict icon size
    
    float width = paddingX * 2.0f + iconSize;
    if (!m_Label.empty()) {
        width += 6.0f; // 6px gap between icon and text
        width += m_Label.length() * 7.5f; // Rough text width estimate for 14px text
    }
    
    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void ToolButton::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolButton::Paint(PaintContext& context) {
    // Update animations
    float targetHover = m_Hovered ? 1.0f : 0.0f;
    float targetPress = m_Pressed ? 1.0f : 0.0f;
    float targetActive = m_Active ? 1.0f : 0.0f;
    
    const float animSpeed = 15.0f;
    m_HoverAnim += (targetHover - m_HoverAnim) * animSpeed * 0.016f;
    m_PressAnim += (targetPress - m_PressAnim) * animSpeed * 0.016f;
    m_ActiveAnim += (targetActive - m_ActiveAnim) * animSpeed * 0.016f;
    
    Rect renderRect = m_Geometry;
    
    // Global button scale animation on press (all buttons shrink to 95%)
    if (m_PressAnim > 0.01f) {
        float scale = 1.0f - (m_PressAnim * 0.05f); // Scale to 95%
        float shrinkW = renderRect.width * (1.0f - scale) * 0.5f;
        float shrinkH = renderRect.height * (1.0f - scale) * 0.5f;
        renderRect.x += shrinkW;
        renderRect.y += shrinkH;
        renderRect.width -= shrinkW * 2.0f;
        renderRect.height -= shrinkH * 2.0f;
    }
    
    // Determine background color based on state
    Color bgColor = m_Style.background.color;
    bgColor.a = 0.0f; // Transparent by default
    
    if (m_ButtonStyle == ToolButtonStyle::PlayButton) {
        bgColor = Color{0.13f, 0.77f, 0.37f, 0.0f}; // Base play button isn't filled until hover
        if (m_HoverAnim > 0.01f) {
            bgColor = Color::Lerp(bgColor, Color{0.13f, 0.77f, 0.37f, 0.2f}, m_HoverAnim); // Soft green highlight
        }
    } else {
    if (m_ActiveAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, Theme::Get().HoverOverlay, m_ActiveAnim);
    }
    if (m_PressAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, Theme::Get().PressedOverlay, m_PressAnim);
    }
    }
    
    // Draw background with rounded corners (8px)
    if (bgColor.a > 0.01f) {
        context.DrawRoundedRect(renderRect, bgColor, Theme::Get().CornerRadiusMedium);
    }
    
    float iconSize = 20.0f;
    
    float contentWidth = iconSize;
    if (!m_Label.empty()) {
        contentWidth += 6.0f + m_Label.length() * 7.5f;
    }
    
    float currentX = renderRect.x + (renderRect.width - contentWidth) / 2.0f;
    float centerY = renderRect.y + renderRect.height / 2.0f;
    
    Color iconColor = Theme::Get().TextSecondary;
    if (m_ButtonStyle == ToolButtonStyle::PlayButton) {
        iconColor = Theme::Get().Success; // Green icon
    } else if (m_ActiveAnim > 0.01f) {
        iconColor = Color::Lerp(Theme::Get().TextPrimary, Theme::Get().SelectedAccent, m_ActiveAnim);
    } else if (m_HoverAnim > 0.01f) {
        iconColor = Color::Lerp(Theme::Get().TextSecondary, Theme::Get().TextPrimary, m_HoverAnim);
    }
    
    int codepoint = Icons::GetCodepoint(m_IconName);
    if (codepoint != 0) {
        context.DrawIcon(codepoint, Point{ currentX, centerY - iconSize/2.0f }, iconColor, iconSize);
    }
    
    currentX += iconSize + 6.0f;
    
    if (!m_Label.empty()) {
        context.DrawText(m_Label, Point{ currentX, centerY - 7.0f }, Theme::Get().TextPrimary, 14.0f);
    }
}

void ToolButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
    }
}

void ToolButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolButton::OnMouseMove(const MouseEvent& event) {
    // Hover state is handled by EventSystem
}

// ToolSeparator implementation
ToolSeparator::ToolSeparator() {}

Size ToolSeparator::Measure(const Size& availableSize) {
    return Size{ SEPARATOR_WIDTH, SEPARATOR_HEIGHT };
}

void ToolSeparator::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolSeparator::Paint(PaintContext& context) {
    float centerY = m_Geometry.y + m_Geometry.height / 2.0f;
    float halfHeight = SEPARATOR_HEIGHT / 2.0f;
    
    Rect lineRect{ m_Geometry.x, centerY - halfHeight, SEPARATOR_WIDTH, SEPARATOR_HEIGHT };
    context.DrawRect(lineRect, Theme::Get().BorderDefault);
}

} // namespace HouseEngine::UI
