#pragma once

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/Icon.h"
#include <string>
#include <functional>

namespace we::runtime::kindui {

// Viewport statistics data structure
struct ViewportStats {
    float fps = 0.0f;
    float gpuTime = 0.0f;
    float cpuTime = 0.0f;
    uint32_t triangles = 0;
    uint32_t drawCalls = 0;
    uint32_t objects = 0;
    std::string renderer;
    std::string atmosphereStatus;
    uint32_t resolutionX = 0;
    uint32_t resolutionY = 0;
    float cameraSpeed = 0.0f;
};

// Viewport overlay widget for displaying stats and controls
class ViewportOverlay : public Widget {
public:
    ViewportOverlay();
    virtual ~ViewportOverlay() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    // Stats management
    void SetStats(const ViewportStats& stats) { m_Stats = stats; }
    const ViewportStats& GetStats() const { return m_Stats; }

    // Visibility toggles
    void SetStatsVisible(bool visible) { m_StatsVisible = visible; }
    void SetGizmoVisible(bool visible) { m_GizmoVisible = visible; }
    void SetNavigationVisible(bool visible) { m_NavigationVisible = visible; }

    // Callbacks
    using OnNavigationAction = std::function<void(const std::string& action)>;
    void SetOnNavigationAction(OnNavigationAction callback) { m_OnNavigationAction = callback; }

private:
    struct NavButton {
        std::string iconName;
        std::string action;
        Rect geometry;
    };

    void CalculateLayout();
    NavButton* GetNavButtonAtPosition(const Point& pos);

    ViewportStats m_Stats;
    
    bool m_StatsVisible = true;
    bool m_GizmoVisible = true;
    bool m_NavigationVisible = true;
    
    Rect m_StatsRect;
    Rect m_GizmoRect;
    Rect m_NavigationRect;
    
    std::vector<NavButton> m_NavButtons;
    
    float m_StatsPadding = 8.0f;
    float m_GizmoSize = 80.0f;
    float m_NavButtonSize = 24.0f;
    float m_NavSpacing = 4.0f;

    OnNavigationAction m_OnNavigationAction;

    WidgetStyle m_Style;
};

// Axis gizmo widget (3D orientation indicator)
class AxisGizmo : public Widget {
public:
    AxisGizmo();
    virtual ~AxisGizmo() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetOrientation(float pitch, float yaw, float roll);

private:
    float m_Pitch = 0.0f;
    float m_Yaw = 0.0f;
    float m_Roll = 0.0f;
    float m_GizmoSize = 80.0f;
};

// Navigation controls widget (camera movement buttons)
class NavigationControls : public Widget {
public:
    NavigationControls();
    virtual ~NavigationControls() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    using OnAction = std::function<void(const std::string& action)>;
    void SetOnAction(OnAction callback) { m_OnAction = callback; }

private:
    struct NavButton {
        std::string iconName;
        std::string action;
        Rect geometry;
        bool hovered = false;
    };

    void CalculateLayout();
    NavButton* GetButtonAtPosition(const Point& pos);

    std::vector<NavButton> m_Buttons;
    OnAction m_OnAction;
    
    float m_ButtonSize = 24.0f;
    float m_Spacing = 4.0f;
};

} // namespace we::editor::viewport::UI
