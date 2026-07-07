#include "Widgets/ViewportWidget.hpp"
#include "Widgets/GraphicsDebuggerPopup.hpp"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "Renderer/Renderer.h"
#include "EditorCamera.hpp"
#include "Scene/Scene.hpp"
#include "Core/PaintContext.hpp"
#include "Rendering/UIRenderer2.hpp"
#include "Core/Theme.hpp"
#include <SDL3/SDL_keyboard.h>
#include <algorithm>
#include <glm/glm.hpp>

namespace we::UI {

ViewportWidget::ViewportWidget(we::runtime::renderer::Renderer* renderer,
                               const std::shared_ptr<we::runtime::engine::EditorCamera>& camera,
                               const std::shared_ptr<we::runtime::scene::Scene>& scene,
                               UIRenderer2* uiRenderer)
    : m_Renderer(renderer), m_Camera(camera), m_Scene(scene), m_uiRenderer(uiRenderer) {
    m_Navigation.SetCamera(camera);
    m_Navigation.SetScene(scene);
    m_Navigation.ApplySettingsFromStore();

    m_GraphicsDebugger = std::make_shared<GraphicsDebuggerPopup>(renderer, camera, scene);
}

void ViewportWidget::Construct() {
    AddChild(m_GraphicsDebugger);
}

ViewportWidget::~ViewportWidget() {
    if (m_uiRenderer && m_ViewportTextureSet != VK_NULL_HANDLE) {
        m_uiRenderer->UnregisterTexture(m_ViewportTextureSet);
        m_ViewportTextureSet = VK_NULL_HANDLE;
    }
}

void ViewportWidget::SetWindow(SDL_Window* window) {
    m_Navigation.SetWindow(window);
}

Size ViewportWidget::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void ViewportWidget::Arrange(const Rect& allottedRect) {
    bool sizeChanged = allottedRect.width != m_Geometry.width || allottedRect.height != m_Geometry.height;
    m_Geometry = allottedRect;
    m_Navigation.SetViewportRect(allottedRect);

    if (sizeChanged && allottedRect.width > 0.0f && allottedRect.height > 0.0f) {
        m_PendingWidth  = static_cast<uint32_t>(allottedRect.width);
        m_PendingHeight = static_cast<uint32_t>(allottedRect.height);
        m_ResizePending = true;
    }

    for (auto& child : m_Children) {
        if (child == m_GraphicsDebugger) {
            continue;
        }
        float compactHeight = 34.0f;
        child->Measure(Size{ allottedRect.width, compactHeight });
        child->Arrange(Rect{ allottedRect.x, allottedRect.y, allottedRect.width, compactHeight });
    }

    if (m_GraphicsDebugger) {
        m_GraphicsDebugger->ArrangeInViewport(allottedRect);
    }
}

void ViewportWidget::FlushPendingResize() {
    if (!m_ResizePending) return;
    m_ResizePending = false;

    if (m_PendingWidth == 0 || m_PendingHeight == 0 || !m_Renderer) return;

    m_Camera->SetViewportSize(
        static_cast<float>(m_PendingWidth),
        static_cast<float>(m_PendingHeight)
    );
}

bool ViewportWidget::HitTestGizmoReset(const Point& position) const {
    Point gizmoCenter = Point{ m_Geometry.x + m_Geometry.width - 55.0f, m_Geometry.y + 55.0f };
    float dx = position.x - gizmoCenter.x;
    float dy = position.y - gizmoCenter.y;
    return dx * dx + dy * dy <= 900.0f;
}

void ViewportWidget::Paint(PaintContext& context) {
    if (!m_Visible) return;

    if (m_ViewportTextureSet != VK_NULL_HANDLE) {
        context.DrawTexture(m_Geometry, m_ViewportTextureSet);
    }

    const Theme& theme = Theme::Get();
    Point gizmoCenter = Point{ m_Geometry.x + m_Geometry.width - 55.0f, m_Geometry.y + 55.0f };
    context.DrawRect(Rect{ gizmoCenter.x - 30.0f, gizmoCenter.y - 30.0f, 60.0f, 60.0f }, theme.GizmoBackground, 30.0f);

    glm::vec3 right = m_Camera->GetRight();
    glm::vec3 up = m_Camera->GetUp();
    glm::vec3 forward = m_Camera->GetForward();

    Color colorX = theme.GizmoAxisX;
    Color colorY = theme.GizmoAxisY;
    Color colorZ = theme.GizmoAxisZ;

    glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
    glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

    Point xProj{ glm::dot(xAxis, right) * 22.0f, -glm::dot(xAxis, up) * 22.0f };
    Point yProj{ glm::dot(yAxis, right) * 22.0f, -glm::dot(yAxis, up) * 22.0f };
    Point zProj{ glm::dot(zAxis, right) * 22.0f, -glm::dot(zAxis, up) * 22.0f };

    struct AxisInfo {
        Point proj;
        Color color;
        std::string label;
        float depth;
    };

    std::vector<AxisInfo> axes = {
        { xProj, colorX, "X", glm::dot(xAxis, forward) },
        { yProj, colorY, "Y", glm::dot(yAxis, forward) },
        { zProj, colorZ, "Z", glm::dot(zAxis, forward) }
    };

    std::sort(axes.begin(), axes.end(), [](const AxisInfo& a, const AxisInfo& b) {
        return a.depth < b.depth;
    });

    for (const auto& axis : axes) {
        Point endPoint{ gizmoCenter.x + axis.proj.x, gizmoCenter.y + axis.proj.y };
        context.DrawLine(gizmoCenter, endPoint, axis.color, 2.5f);
        context.DrawRect(Rect{ endPoint.x - 3.0f, endPoint.y - 3.0f, 6.0f, 6.0f }, axis.color, 3.0f);
        context.DrawText(axis.label, Point{ endPoint.x + 4.0f, endPoint.y - 6.0f }, axis.color, 10.0f);
    }

    if (m_Navigation.IsFlyLookActive()) {
        const float centerX = m_Geometry.x + m_Geometry.width * 0.5f;
        const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
        const Color crosshairColor{ 1.0f, 1.0f, 1.0f, 0.9f };
        constexpr float kArm = 8.0f;
        constexpr float kThickness = 1.5f;
        context.DrawLine(
            Point{ centerX - kArm, centerY },
            Point{ centerX + kArm, centerY },
            crosshairColor,
            kThickness);
        context.DrawLine(
            Point{ centerX, centerY - kArm },
            Point{ centerX, centerY + kArm },
            crosshairColor,
            kThickness);
    }

    for (auto& child : m_Children) {
        child->Paint(context);
    }
}

void ViewportWidget::OnMouseDown(const MouseEvent& event) {
    if (HitTestGizmoReset(event.position)) {
        m_Camera->Reset();
        return;
    }

    if (event.type == MouseEventType::MouseDown
        && event.button == MouseButton::Left
        && event.ctrlDown
        && m_Geometry.Contains(event.position)
        && m_Renderer) {
        return;
    }

    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        auto& child = *it;
        const Rect geom = child->GetGeometry();
        if (geom.width > 0.0f && geom.height > 0.0f
            && event.position.x >= geom.x && event.position.x <= geom.x + geom.width
            && event.position.y >= geom.y && event.position.y <= geom.y + geom.height) {
            child->OnMouseDown(event);
            return;
        }
    }

    if (m_Geometry.Contains(event.position)
        && we::programs::editor::PlaceActorsPlacement::Get().HasActivePlacement()) {
        const float localX = event.position.x - m_Geometry.x;
        const float localY = event.position.y - m_Geometry.y;
        if (we::programs::editor::PlaceActorsPlacement::Get().TryPlaceAtViewportPoint(localX, localY)) {
            return;
        }
    }

    m_Navigation.OnMouseDown(event);
}

void ViewportWidget::OnMouseMove(const MouseEvent& event) {
    m_Navigation.OnMouseMove(event);

    for (auto& child : m_Children) {
        child->OnMouseMove(event);
    }
}

void ViewportWidget::OnMouseUp(const MouseEvent& event) {
    m_Navigation.OnMouseUp(event);

    for (auto& child : m_Children) {
        child->OnMouseUp(event);
    }
}

void ViewportWidget::OnMouseWheel(const MouseEvent& event) {
    m_Navigation.OnMouseWheel(event);
}

void ViewportWidget::OnKeyDown(const KeyEvent& event) {
    m_Navigation.OnKeyDown(event);
}

void ViewportWidget::Tick(float deltaTime) {
    m_FPS = 1.0f / (deltaTime > 0.0f ? deltaTime : 0.016f);
    m_FrameTime = deltaTime * 1000.0f;
    if (m_GraphicsDebugger) {
        m_GraphicsDebugger->SetFrameStats(m_FPS, m_FrameTime);
    }
    m_Navigation.Tick(deltaTime);
}

} // namespace we::UI
