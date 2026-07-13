#include "Widgets/ViewportOverlay.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "Rendering/IconMetrics.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

ViewportOverlay::ViewportOverlay()
    : m_Style(WidgetStyle::Panel())
{
    // Initialize navigation buttons
    m_NavButtons = {
        { Icons::PlusName, "zoom-in", Rect{} },
        { Icons::MinusName, "zoom-out", Rect{} },
        { Icons::CameraName, "reset-camera", Rect{} }
    };
}

Size ViewportOverlay::Measure(const Size& availableSize) {
    CalculateLayout();
    return availableSize;
}

void ViewportOverlay::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateLayout();
}

void ViewportOverlay::Paint(PaintContext& context) {
    // Draw stats panel (top-left)
    if (m_StatsVisible) {
        context.DrawRoundedRect(m_StatsRect, ThemeColor(ThemeToken::GizmoBackground), 4.0f);
        
        float y = m_StatsRect.y + m_StatsPadding;
        float lineHeight = 14.0f;
        
        // Draw stats text
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", m_Stats.fps);
        context.DrawText(fpsText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextPrimary), 12.0f);
        y += lineHeight;
        
        char gpuText[32];
        snprintf(gpuText, sizeof(gpuText), "GPU: %.2f ms", m_Stats.gpuTime);
        context.DrawText(gpuText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        y += lineHeight;
        
        char cpuText[32];
        snprintf(cpuText, sizeof(cpuText), "CPU: %.2f ms", m_Stats.cpuTime);
        context.DrawText(cpuText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        y += lineHeight;
        
        char triText[32];
        snprintf(triText, sizeof(triText), "Tris: %u", m_Stats.triangles);
        context.DrawText(triText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        y += lineHeight;
        
        char drawText[32];
        snprintf(drawText, sizeof(drawText), "Draws: %u", m_Stats.drawCalls);
        context.DrawText(drawText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        y += lineHeight;
        
        char objText[32];
        snprintf(objText, sizeof(objText), "Objects: %u", m_Stats.objects);
        context.DrawText(objText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        y += lineHeight;
        
        char resText[32];
        snprintf(resText, sizeof(resText), "%ux%u", m_Stats.resolutionX, m_Stats.resolutionY);
        context.DrawText(resText, Point{ m_StatsRect.x + m_StatsPadding, y }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        y += lineHeight;

        if (!m_Stats.atmosphereStatus.empty()) {
            context.DrawText(
                m_Stats.atmosphereStatus.c_str(),
                Point{ m_StatsRect.x + m_StatsPadding, y },
                m_Stats.atmosphereStatus.find("MISSING") != std::string::npos
                    ? ThemeColor(ThemeToken::ErrorForeground)
                    : ThemeColor(ThemeToken::TextSecondary),
                12.0f);
            y += lineHeight;
        }
    }
    
    // Draw axis gizmo (top-right)
    if (m_GizmoVisible) {
        context.DrawRoundedRect(m_GizmoRect, ThemeColor(ThemeToken::GizmoBackground), 4.0f);
        
        // Draw simplified axis representation
        float centerX = m_GizmoRect.x + m_GizmoRect.width / 2.0f;
        float centerY = m_GizmoRect.y + m_GizmoRect.height / 2.0f;
        float axisLength = 25.0f;
        
        // X axis (red)
        context.DrawLine(Point{ centerX, centerY }, Point{ centerX + axisLength, centerY }, ThemeColor(ThemeToken::GizmoAxisX), 2.0f);
        context.DrawText("X", Point{ centerX + axisLength + 2.0f, centerY - 6.0f }, ThemeColor(ThemeToken::GizmoAxisX), 10.0f);
        
        // Y axis (green)
        context.DrawLine(Point{ centerX, centerY }, Point{ centerX, centerY - axisLength }, ThemeColor(ThemeToken::GizmoAxisY), 2.0f);
        context.DrawText("Y", Point{ centerX - 3.0f, centerY - axisLength - 8.0f }, ThemeColor(ThemeToken::GizmoAxisY), 10.0f);
        
        // Z axis (blue)
        context.DrawLine(Point{ centerX, centerY }, Point{ centerX - axisLength * 0.5f, centerY + axisLength * 0.5f }, ThemeColor(ThemeToken::GizmoAxisZ), 2.0f);
        context.DrawText("Z", Point{ centerX - axisLength * 0.5f - 8.0f, centerY + axisLength * 0.5f + 2.0f }, ThemeColor(ThemeToken::GizmoAxisZ), 10.0f);
    }
    
    // Draw navigation controls (bottom-right)
    if (m_NavigationVisible) {
        context.DrawRoundedRect(m_NavigationRect, ThemeColor(ThemeToken::GizmoBackground), 4.0f);
        
        for (const auto& btn : m_NavButtons) {
            const float tier = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeNavigation));
            IconPainter::DrawIcon(context, btn.iconName,
                IconMetrics::PlaceGlyphCentered(btn.geometry, tier),
                ThemeColor(ThemeToken::TextPrimary));
        }
    }
}

void ViewportOverlay::OnMouseDown(const MouseEvent& event) {
    NavButton* btn = GetNavButtonAtPosition(event.position);
    if (btn && m_OnNavigationAction) {
        m_OnNavigationAction(btn->action);
    }
}

void ViewportOverlay::OnMouseMove(const MouseEvent& event) {
    // Update hover states for navigation buttons
    for (auto& btn : m_NavButtons) {
        bool hovered = btn.geometry.Contains(event.position);
        // Hover state could trigger visual feedback
    }
}

void ViewportOverlay::CalculateLayout() {
    // Stats panel (top-left)
    if (m_StatsVisible) {
        float statsWidth = 160.0f;
        float statsHeight = 120.0f;
        m_StatsRect = Rect{
            m_Geometry.x + 8.0f,
            m_Geometry.y + 8.0f,
            statsWidth,
            statsHeight
        };
    }
    
    // Axis gizmo (top-right)
    if (m_GizmoVisible) {
        m_GizmoRect = Rect{
            m_Geometry.x + m_Geometry.width - m_GizmoSize - 16.0f,
            m_Geometry.y + 16.0f,
            m_GizmoSize,
            m_GizmoSize
        };
    }
    
    // Navigation controls (bottom-right)
    if (m_NavigationVisible) {
        float navWidth = static_cast<float>(m_NavButtons.size()) * (m_NavButtonSize + m_NavSpacing) + m_NavSpacing;
        float navHeight = m_NavButtonSize + m_NavSpacing * 2.0f;
        
        m_NavigationRect = Rect{
            m_Geometry.x + m_Geometry.width - navWidth - 8.0f,
            m_Geometry.y + m_Geometry.height - navHeight - 8.0f,
            navWidth,
            navHeight
        };
        
        // Calculate button positions
        float btnX = m_NavigationRect.x + m_NavSpacing;
        float btnY = m_NavigationRect.y + m_NavSpacing;
        
        for (auto& btn : m_NavButtons) {
            btn.geometry = Rect{ btnX, btnY, m_NavButtonSize, m_NavButtonSize };
            btnX += m_NavButtonSize + m_NavSpacing;
        }
    }
}

ViewportOverlay::NavButton* ViewportOverlay::GetNavButtonAtPosition(const Point& pos) {
    for (auto& btn : m_NavButtons) {
        if (btn.geometry.Contains(pos)) {
            return &btn;
        }
    }
    return nullptr;
}

// AxisGizmo implementation
AxisGizmo::AxisGizmo() {}

Size AxisGizmo::Measure(const Size& availableSize) {
    return Size{ m_GizmoSize, m_GizmoSize };
}

void AxisGizmo::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void AxisGizmo::Paint(PaintContext& context) {
    context.DrawRoundedRect(m_Geometry, ThemeColor(ThemeToken::GizmoBackground), 4.0f);
    
    float centerX = m_Geometry.x + m_Geometry.width / 2.0f;
    float centerY = m_Geometry.y + m_Geometry.height / 2.0f;
    float axisLength = 25.0f;
    
    // Draw axes with orientation
    // X axis (red)
    context.DrawLine(Point{ centerX, centerY }, Point{ centerX + axisLength, centerY }, ThemeColor(ThemeToken::GizmoAxisX), 2.0f);
    
    // Y axis (green)
    context.DrawLine(Point{ centerX, centerY }, Point{ centerX, centerY - axisLength }, ThemeColor(ThemeToken::GizmoAxisY), 2.0f);
    
    // Z axis (blue)
    context.DrawLine(Point{ centerX, centerY }, Point{ centerX - axisLength * 0.5f, centerY + axisLength * 0.5f }, ThemeColor(ThemeToken::GizmoAxisZ), 2.0f);
}

void AxisGizmo::SetOrientation(float pitch, float yaw, float roll) {
    m_Pitch = pitch;
    m_Yaw = yaw;
    m_Roll = roll;
}

// NavigationControls implementation
NavigationControls::NavigationControls() {
    m_Buttons = {
        { Icons::PlusName, "zoom-in", Rect{}, false },
        { Icons::MinusName, "zoom-out", Rect{}, false },
        { Icons::CameraName, "reset", Rect{}, false }
    };
}

Size NavigationControls::Measure(const Size& availableSize) {
    float totalWidth = static_cast<float>(m_Buttons.size()) * (m_ButtonSize + m_Spacing) + m_Spacing;
    return Size{ totalWidth, m_ButtonSize + m_Spacing * 2.0f };
}

void NavigationControls::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateLayout();
}

void NavigationControls::Paint(PaintContext& context) {
    context.DrawRoundedRect(m_Geometry, ThemeColor(ThemeToken::GizmoBackground), 4.0f);
    
    for (const auto& btn : m_Buttons) {
        Color iconColor = btn.hovered ? ThemeColor(ThemeToken::AccentPrimary) : ThemeColor(ThemeToken::TextPrimary);
        const float tier = static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeNavigation));
        IconPainter::DrawIcon(context, btn.iconName,
            IconMetrics::PlaceGlyphCentered(btn.geometry, tier), iconColor);
    }
}

void NavigationControls::OnMouseDown(const MouseEvent& event) {
    NavButton* btn = GetButtonAtPosition(event.position);
    if (btn && m_OnAction) {
        m_OnAction(btn->action);
    }
}

void NavigationControls::OnMouseMove(const MouseEvent& event) {
    for (auto& btn : m_Buttons) {
        btn.hovered = btn.geometry.Contains(event.position);
    }
}

void NavigationControls::CalculateLayout() {
    float x = m_Geometry.x + m_Spacing;
    float y = m_Geometry.y + m_Spacing;
    
    for (auto& btn : m_Buttons) {
        btn.geometry = Rect{ x, y, m_ButtonSize, m_ButtonSize };
        x += m_ButtonSize + m_Spacing;
    }
}

NavigationControls::NavButton* NavigationControls::GetButtonAtPosition(const Point& pos) {
    for (auto& btn : m_Buttons) {
        if (btn.geometry.Contains(pos)) {
            return &btn;
        }
    }
    return nullptr;
}

} // namespace we::editor::viewport::UI
