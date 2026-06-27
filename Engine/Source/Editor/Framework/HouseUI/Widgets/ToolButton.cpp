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
    if (m_ButtonStyle == ToolButtonStyle::WindowControl || m_ButtonStyle == ToolButtonStyle::WindowClose) {
        m_DesiredSize = Size{ 40.0f, 30.0f }; // Compact hit area for window controls
        return m_DesiredSize;
    }
    
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        m_DesiredSize = Size{ 30.0f, 30.0f }; // 30x30 hit area matching standard
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        float width = 30.0f; // 30x30 standard hit area
        if (m_IsDropdown) width += 16.0f; // 10px chevron + 6px spacing
        m_DesiredSize = Size{ width, 30.0f }; // Strict 30px height
        return m_DesiredSize;
    }

    float height = 30.0f; // AAA 30px button standard
    float paddingX = 8.0f; // 8px horizontal padding
    float iconSize = 16.0f; // 16px icon size
    
    float width = paddingX * 2.0f + iconSize; // base width
    if (!m_Label.empty()) {
        width += 4.0f; // gap between icon and text compressed to 4px
        width += m_Label.length() * 7.5f; // Rough text width estimate
    }
    
    if (m_IsDropdown) {
        width += 14.0f; // 8px chevron + 6px spacing from text
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
    
    const float animSpeed = 8.33f; // ~120ms transition speed
    m_HoverAnim += (targetHover - m_HoverAnim) * animSpeed * 0.016f;
    m_PressAnim += (targetPress - m_PressAnim) * animSpeed * 0.016f;
    m_ActiveAnim += (targetActive - m_ActiveAnim) * animSpeed * 0.016f;
    
    Rect renderRect = m_Geometry;
    
    bool isWindowControl = (m_ButtonStyle == ToolButtonStyle::WindowControl || m_ButtonStyle == ToolButtonStyle::WindowClose);
    bool isTitleBarTool = (m_ButtonStyle == ToolButtonStyle::TitleBarTool);
    
    // Global button scale animation removed for AAA clean appearance
    
    float iconSize = 24.0f; 
    if (isWindowControl) iconSize = 12.0f;
    if (isTitleBarTool) iconSize = 16.0f; // Uniform 16px
    
    // For toolbar tools, iconSize is fixed to 16, play buttons slightly larger
    bool isToolbarIcon = (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly);
    bool isPlayButton = (m_ButtonStyle == ToolButtonStyle::PlayButton);
    bool isNormalButton = (m_ButtonStyle == ToolButtonStyle::Normal);
    if (isToolbarIcon || isNormalButton) iconSize = 16.0f;
    if (isPlayButton) iconSize = 17.9f; // Approximately 12% larger than 16px
    
    float contentWidth = iconSize;
    if (!m_Label.empty()) {
        contentWidth += 4.0f + m_Label.length() * 7.5f;
    }
    
    float currentX = renderRect.x + (renderRect.width - contentWidth) / 2.0f;
    float centerY = renderRect.y + renderRect.height / 2.0f;
    
    if (isWindowControl) {
        if (m_HoverAnim > 0.01f) {
            Color hoverBg = (m_ButtonStyle == ToolButtonStyle::WindowClose) 
                            ? Color{ 0.8f, 0.2f, 0.2f, m_HoverAnim } // Red hover for close
                            : Color{ 0.3f, 0.3f, 0.3f, m_HoverAnim * 0.5f }; // Subtle dark gray
            context.DrawRect(renderRect, hoverBg);
        }
        
        Color iconColor = Color{ 0.72f, 0.72f, 0.72f, 1.0f }; // #B8B8B8 Muted gray
        if (m_ButtonStyle == ToolButtonStyle::WindowClose && m_HoverAnim > 0.5f) {
            iconColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f }; // White when hovering close
        }
        
        int codepoint = Icons::GetCodepoint(m_IconName);
        if (codepoint != 0) {
            context.DrawIcon(codepoint, Point{ currentX, centerY - iconSize/2.0f }, iconColor, iconSize);
        }
    } else {
        if (isToolbarIcon || isPlayButton || isNormalButton) {
            // Uncluttered AAA Desktop Style (UE5 Starship, Rider)
            // Borderless by default. Hover/press get subtle backgrounds.
            // Dropdowns get a thin 1px border.
            
            Color bg = Color{ 0.0f, 0.0f, 0.0f, 0.0f }; // Transparent by default
            bool drawBg = false;

            if (m_PressAnim > 0.01f) {
                // Pressed #3A3A3A
                bg = Color{ 0.227f, 0.227f, 0.227f, 1.0f };
                drawBg = true;
            } else if (m_HoverAnim > 0.01f) {
                // Hover #323232
                bg = Color{ 0.196f, 0.196f, 0.196f, 1.0f };
                drawBg = true;
            } else if (m_Active) {
                // Checked #3B82F6
                bg = Color{ 0.231f, 0.510f, 0.965f, 1.0f };
                drawBg = true;
            }

            if (drawBg) {
                context.DrawRoundedRect(renderRect, bg, 4.0f); // 4px corner radius for modern AAA look
            }
            
            // Dropdowns get no border in AAA, or just the same hover effect
            if (m_IsDropdown && drawBg) {
                // No extra border, just draw background
            }
        }
        
        // Icon color rules:
        // - PlayButton style: ALWAYS blue (#4DA3FF) — the only permanently colored tool
        // - Active (e.g., selected transform): blue
        // - Hovered: white
        // - Default: neutral gray #BFBFBF
        Color iconColor;
        if (isPlayButton) {
            // Play is the ONLY permanently colored icon
            iconColor = Color{ 0.30f, 0.64f, 1.0f, 1.0f }; // #4DA3FF blue always
            if (m_HoverAnim > 0.01f) {
                // Slightly brighter blue on hover
                iconColor = Color{ 0.45f, 0.76f, 1.0f, 1.0f };
            }
            if (m_PressAnim > 0.01f) {
                iconColor = Color{ 0.20f, 0.50f, 0.90f, 1.0f }; // Darker blue on press
            }
        } else {
            iconColor = Color{ 0.75f, 0.75f, 0.75f, 1.0f }; // #BFBFBF neutral gray
            if (m_HoverAnim > 0.01f) {
                iconColor = Color::Lerp(iconColor, Color::White(), m_HoverAnim); // fade to white on hover
            }
            if (m_Active || (m_PressAnim > 0.01f)) {
                iconColor = Color{ 0.30f, 0.64f, 1.0f, 1.0f }; // #4DA3FF when active/pressed
            }
        }
        
        float actualIconSize = iconSize; // Use the dynamically set iconSize (16px or 18px)
        
        int codepoint = Icons::GetCodepoint(m_IconName);
        if (codepoint != 0) {
            float drawX = currentX;
            if (m_Label.empty()) {
                // Center icon in its designated space
                float spaceWidth = renderRect.width;
                if (m_IsDropdown) spaceWidth -= 16.0f; // Leave space for chevron
                drawX = renderRect.x + (spaceWidth - actualIconSize) / 2.0f;
            } else {
                drawX = renderRect.x + 8.0f; // 8px left padding when there is text
            }
            context.DrawIcon(codepoint, Point{ drawX, centerY - actualIconSize/2.0f }, iconColor, actualIconSize);
            currentX = drawX + actualIconSize + 6.0f; // 6px gap before text
        }
        
        if (!m_Label.empty()) {
            Color textColor = iconColor;
            context.DrawText(m_Label, Point{ currentX, centerY - (Theme::Get().TextSizeToolbar / 2.0f) }, textColor, Theme::Get().TextSizeToolbar);
        }
        
        if (m_IsDropdown) {
            int chevronCodepoint = Icons::GetCodepoint(Icons::ChevronDownName);
            if (chevronCodepoint != 0) {
                float chevronSize = 8.0f; // 8px exact size
                float chevronX = renderRect.x + renderRect.width - 8.0f - chevronSize; // paddingX is 8px
                context.DrawIcon(chevronCodepoint, Point{ chevronX, centerY - chevronSize/2.0f }, iconColor, chevronSize);
                // Bold effect:
                context.DrawIcon(chevronCodepoint, Point{ chevronX + 0.5f, centerY - chevronSize/2.0f }, iconColor, chevronSize);
            }
        }
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
