#include "Widgets/ToolButton.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include <algorithm>

namespace we::UI {

ToolButton::ToolButton(const std::string& iconName, const std::string& label, std::function<void()> onClicked, const std::string& tooltip)
    : m_IconName(iconName)
    , m_Label(label)
    , m_Tooltip(tooltip)
    , m_OnClicked(onClicked)
    , m_Style(WidgetStyle::ToolButton())
{}

Size ToolButton::Measure(const Size& availableSize) {
    if (m_ButtonStyle == ToolButtonStyle::WindowControl || m_ButtonStyle == ToolButtonStyle::WindowClose) {
        m_DesiredSize = Size{ 30.0f, 30.0f };
        return m_DesiredSize;
    }
    
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        m_DesiredSize = Size{ 26.0f, 26.0f };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarInline) {
        const float padH     = 4.0f;
        const float iconSz   = 14.0f;
        const float iconGap  = 4.0f;
        const float chevW    = 10.0f;
        const float textSize = 13.0f;
        float textW = m_Label.empty() ? 0.0f : m_Label.length() * textSize * 0.6f;
        float width = padH + iconSz + (m_Label.empty() ? 0.0f : iconGap + textW);
        if (m_IsDropdown) width += 4.0f + chevW;
        width += padH;
        m_DesiredSize = Size{ width, 28.0f };
        return m_DesiredSize;
    }

    // TransportButton: Play / Pause / Stop – compact 24×24 hit area
    if (m_ButtonStyle == ToolButtonStyle::TransportButton || m_ButtonStyle == ToolButtonStyle::PlayButton) {
        m_DesiredSize = Size{ 24.0f, 24.0f };
        return m_DesiredSize;
    }

    if (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly) {
        m_DesiredSize = Size{ 24.0f, 24.0f };
        return m_DesiredSize;
    }

    // Normal – used for labeled dropdowns (Platform, Settings)
    const float height  = 30.0f;
    const float padL    = 12.0f; // left padding (icon start) – 12px for breathing room
    const float padR    = 10.0f; // right padding (after chevron) – 10px
    const float iconSz  = 16.0f;
    const float iconGap = 6.0f;  // gap between icon and text
    const float chevW   = 16.0f; // chevron area (icon 8px + gap 8px)

    float width = padL + iconSz;
    if (!m_Label.empty()) {
        width += iconGap + m_Label.length() * 7.2f;
    }
    if (m_IsDropdown) {
        width += chevW;
    }
    width += padR;

    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void ToolButton::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ToolButton::Paint(PaintContext& context) {
    // Animate hover / press / active
    const float animSpeed = 8.33f;
    m_HoverAnim  += ((m_Hovered ? 1.0f : 0.0f) - m_HoverAnim)  * animSpeed * 0.016f;
    m_PressAnim  += ((m_Pressed ? 1.0f : 0.0f) - m_PressAnim)  * animSpeed * 0.016f;
    m_ActiveAnim += ((m_Active  ? 1.0f : 0.0f) - m_ActiveAnim) * animSpeed * 0.016f;

    Rect renderRect = m_Geometry;
    float centerY   = renderRect.y + renderRect.height / 2.0f;

    const bool isWindowControl = (m_ButtonStyle == ToolButtonStyle::WindowControl ||
                                   m_ButtonStyle == ToolButtonStyle::WindowClose);
    const bool isTransport     = (m_ButtonStyle == ToolButtonStyle::TransportButton ||
                                   m_ButtonStyle == ToolButtonStyle::PlayButton);
    const bool isToolbarIcon   = (m_ButtonStyle == ToolButtonStyle::ToolbarIconOnly);
    const bool isInline        = (m_ButtonStyle == ToolButtonStyle::ToolbarInline);
    const bool isNormal        = (m_ButtonStyle == ToolButtonStyle::Normal);

    // ── Window controls (Minimize / Maximize / Close) ────────────────────────
    if (isWindowControl) {
        if (m_HoverAnim > 0.01f) {
            Color hoverBg = (m_ButtonStyle == ToolButtonStyle::WindowClose)
                            ? Color{ 0.8f, 0.2f, 0.2f, m_HoverAnim }
                            : Color{ 0.196f, 0.196f, 0.196f, m_HoverAnim };
            context.DrawRoundedRect(renderRect, hoverBg, 4.0f);
        }
        Color iconColor = Color{ 0.69f, 0.69f, 0.69f, 1.0f };
        if (m_HoverAnim > 0.01f) iconColor = Color::Lerp(iconColor, Color::White(), m_HoverAnim);
        if (m_ButtonStyle == ToolButtonStyle::WindowClose && m_HoverAnim > 0.5f)
            iconColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        float iconSize = 16.0f;
        float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
        int cp = Icons::GetCodepoint(m_IconName);
        if (cp != 0) context.DrawIcon(cp, Point{ drawX, centerY - iconSize / 2.0f }, iconColor, iconSize);
        return;
    }

    // ── Transport buttons (Play / Pause / Stop) ──────────────────────────────
    if (isTransport) {
        // Hover / press: subtle rounded rect background
        if (m_PressAnim > 0.01f) {
            context.DrawRoundedRect(renderRect, Theme::Get().PressedOverlay, 3.0f);
        } else if (m_HoverAnim > 0.01f) {
            Color hbg = Color::Lerp(Color{0,0,0,0}, Theme::Get().HoverButton, m_HoverAnim);
            context.DrawRoundedRect(renderRect, hbg, 3.0f);
        } else if (m_Active) {
            context.DrawRoundedRect(renderRect, Theme::Get().SelectedBg, 3.0f);
        }
        // 18px icon centered in the 24px hit area
        const float iconSize = 18.0f;
        Color iconColor = Theme::Get().TextSecondary;
        if (m_HoverAnim > 0.01f)
            iconColor = Color::Lerp(iconColor, Theme::Get().TextPrimary, m_HoverAnim);
        if (m_Active || m_PressAnim > 0.5f)
            iconColor = Theme::Get().SelectedAccent; // amber when active
        float drawX = renderRect.x + (renderRect.width  - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
        int cp = Icons::GetCodepoint(m_IconName);
        if (cp != 0) context.DrawIcon(cp, Point{ drawX, drawY }, iconColor, iconSize);
        return;
    }

    // ── TitleBar tool buttons ────────────────────────────────────────────────
    if (m_ButtonStyle == ToolButtonStyle::TitleBarTool) {
        if (m_HoverAnim > 0.01f) {
            Color hbg = Color::Lerp(Color{0,0,0,0}, Theme::Get().HoverButton, m_HoverAnim);
            context.DrawRoundedRect(renderRect, hbg, 3.0f);
        }
        const float iconSize = 15.0f;
        Color iconColor = Theme::Get().TextSecondary;
        if (m_HoverAnim > 0.01f) iconColor = Color::Lerp(iconColor, Theme::Get().TextPrimary, m_HoverAnim);
        if (m_Active || m_PressAnim > 0.01f) iconColor = Theme::Get().SelectedAccent;
        float drawX = renderRect.x + (renderRect.width  - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
        int cp = Icons::GetCodepoint(m_IconName);
        if (cp != 0) context.DrawIcon(cp, Point{ drawX, drawY }, iconColor, iconSize);
        return;
    }

    // ── Inline toolbar items (Platform / Settings) – transparent, hover only ─
    if (isInline) {
        if (m_PressAnim > 0.01f) {
            context.DrawRoundedRect(renderRect, Theme::Get().PressedOverlay, 3.0f);
        } else if (m_HoverAnim > 0.01f) {
            Color hbg = Color::Lerp(Color{0,0,0,0}, Theme::Get().HoverButton, m_HoverAnim);
            context.DrawRoundedRect(renderRect, hbg, 3.0f);
        }

        const float iconSize  = 14.0f;
        const float textSize  = 13.0f; // matches menu bar typography
        const float iconGap   = 4.0f;
        Color textColor = Theme::Get().TextPrimary;
        if (m_HoverAnim > 0.01f) textColor = Color::White();

        float currentX = renderRect.x + 4.0f;
        int cp = Icons::GetCodepoint(m_IconName);
        if (cp != 0) {
            context.DrawIcon(cp, Point{ currentX, centerY - iconSize / 2.0f }, textColor, iconSize);
            currentX += iconSize + iconGap;
        }

        if (!m_Label.empty()) {
            context.DrawText(m_Label, Point{ currentX, centerY - textSize / 2.0f }, textColor, textSize);
            currentX += m_Label.length() * textSize * 0.6f + 4.0f;
        }

        if (m_IsDropdown) {
            int chevCp = Icons::GetCodepoint(Icons::ChevronDownName);
            if (chevCp != 0) {
                const float chevSize = 8.0f;
                context.DrawIcon(chevCp, Point{ currentX, centerY - chevSize / 2.0f }, textColor, chevSize);
            }
        }
        return;
    }

    // ── Toolbar icon-only buttons ────────────────────────────────────────────
    if (isToolbarIcon) {
        if (m_PressAnim > 0.01f) {
            context.DrawRoundedRect(renderRect, Theme::Get().PressedOverlay, 3.0f);
        } else if (m_HoverAnim > 0.01f) {
            Color hbg = Color::Lerp(Color{0,0,0,0}, Theme::Get().HoverButton, m_HoverAnim);
            context.DrawRoundedRect(renderRect, hbg, 3.0f);
        } else if (m_Active) {
            context.DrawRoundedRect(renderRect, Theme::Get().SelectedBg, 3.0f);
        }
        const float iconSize = 16.0f;
        Color iconColor = Theme::Get().TextSecondary;
        if (m_HoverAnim > 0.01f) iconColor = Color::Lerp(iconColor, Theme::Get().TextPrimary, m_HoverAnim);
        if (m_Active || m_PressAnim > 0.01f) iconColor = Theme::Get().SelectedAccent;
        float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
        float drawY = renderRect.y + (renderRect.height - iconSize) / 2.0f;
        int cp = Icons::GetCodepoint(m_IconName);
        if (cp != 0) context.DrawIcon(cp, Point{ drawX, drawY }, iconColor, iconSize);
        return;
    }

    // ── Normal labeled buttons (legacy dropdown style) ───────────────────────
    if (isNormal) {
        const bool isDropdownControl = m_IsDropdown;
        Color bg{ 0.0f, 0.0f, 0.0f, 0.0f };
        bool drawBg = false;
        if (m_PressAnim > 0.01f) {
            bg = Theme::Get().PressedOverlay; drawBg = true;
        } else if (m_HoverAnim > 0.01f) {
            bg = Color::Lerp(Color{0,0,0,0}, Theme::Get().HoverButton, m_HoverAnim); drawBg = true;
        } else if (m_Active) {
            bg = Theme::Get().SelectedBg; drawBg = true;
        } else if (isDropdownControl) {
            bg = Color{ 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
            drawBg = true;
        }

        if (drawBg) context.DrawRoundedRect(renderRect, bg, 4.0f);

        if (isDropdownControl) {
            Color borderCol = m_HoverAnim > 0.01f
                ? Color{ 0.298f, 0.298f, 0.298f, 1.0f }  // #4C4C4C
                : Color{ 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A
            context.DrawRoundedRectOutline(renderRect, borderCol, 1.0f, 4.0f);
        }

        // Icon color
        Color iconColor = Theme::Get().TextSecondary;
        if (m_HoverAnim > 0.01f) iconColor = Color::Lerp(iconColor, Theme::Get().TextPrimary, m_HoverAnim);
        if (m_Active || m_PressAnim > 0.01f) iconColor = Theme::Get().SelectedAccent;

        const float iconSize = 16.0f;
        float currentX;

        int cp = Icons::GetCodepoint(m_IconName);
        if (cp != 0) {
            if (m_Label.empty() && !m_IsDropdown) {
                float drawX = renderRect.x + (renderRect.width - iconSize) / 2.0f;
                context.DrawIcon(cp, Point{ drawX, centerY - iconSize / 2.0f }, iconColor, iconSize);
            } else if (m_Label.empty() && m_IsDropdown) {
                currentX = renderRect.x + 8.0f;
                context.DrawIcon(cp, Point{ currentX, centerY - iconSize / 2.0f }, iconColor, iconSize);
            } else {
                currentX = renderRect.x + 12.0f;
                context.DrawIcon(cp, Point{ currentX, centerY - iconSize / 2.0f }, iconColor, iconSize);
                currentX += iconSize + 6.0f;
            }
        } else {
            currentX = renderRect.x + 12.0f;
        }

        if (!m_Label.empty()) {
            const float textSize = Theme::Get().TextSizeToolbar;
            context.DrawText(m_Label, Point{ currentX, centerY - textSize / 2.0f }, iconColor, textSize);
        }

        // Chevron for dropdowns – sits 10px from right edge
        if (m_IsDropdown) {
            int chevCp = Icons::GetCodepoint(Icons::ChevronDownName);
            if (chevCp != 0) {
                const float chevSize = 8.0f;
                float chevX = renderRect.x + renderRect.width - 10.0f - chevSize;
                context.DrawIcon(chevCp, Point{ chevX, centerY - chevSize / 2.0f }, iconColor, chevSize);
                // Sub-pixel bold pass for crispness
                context.DrawIcon(chevCp, Point{ chevX + 0.5f, centerY - chevSize / 2.0f }, iconColor, chevSize);
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

} // namespace we::editor::toolbar::UI
