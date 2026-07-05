#include "Widgets/ViewportWidget.hpp"
#include "Renderer/FrameStats.hpp"
#include "Renderer/RenderDiagnostics.hpp"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "Renderer/Renderer.hpp"
#include "EditorCamera.hpp"
#include "Scene/Scene.hpp"
#include "Core/PaintContext.hpp"
#include "Rendering/UIRenderer.hpp"
#include "Core/Theme.hpp"
#include <SDL3/SDL_keyboard.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iomanip>
#include <sstream>

namespace we::UI {

ViewportWidget::ViewportWidget(const std::shared_ptr<we::runtime::renderer::Renderer>& renderer,
                               const std::shared_ptr<we::runtime::engine::EditorCamera>& camera,
                               const std::shared_ptr<we::runtime::scene::Scene>& scene,
                               UIRenderer* uiRenderer)
    : m_Renderer(renderer), m_Camera(camera), m_Scene(scene), m_uiRenderer(uiRenderer) {
    m_Navigation.SetCamera(camera);
    m_Navigation.SetScene(scene);
    m_Navigation.ApplySettingsFromStore();
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

    if (!m_Children.empty()) {
        auto toolbar = m_Children[0];
        float compactHeight = 34.0f;
        toolbar->Measure(Size{ allottedRect.width, compactHeight });
        toolbar->Arrange(Rect{ allottedRect.x, allottedRect.y, allottedRect.width, compactHeight });
    }
}

void ViewportWidget::FlushPendingResize() {
    if (!m_ResizePending) return;
    m_ResizePending = false;

    if (m_PendingWidth == 0 || m_PendingHeight == 0) return;

    m_Renderer->GetOffscreenFramebuffer().Resize(
        m_PendingWidth,
        m_PendingHeight,
        m_Renderer->GetOffscreenRenderPass()
    );
    m_Camera->SetViewportSize(
        static_cast<float>(m_PendingWidth),
        static_cast<float>(m_PendingHeight)
    );

    if (m_uiRenderer) {
        if (m_ViewportTextureSet != VK_NULL_HANDLE) {
            m_uiRenderer->UnregisterTexture(m_ViewportTextureSet);
            m_ViewportTextureSet = VK_NULL_HANDLE;
        }
        m_ViewportTextureSet = m_uiRenderer->RegisterTexture(
            m_Renderer->GetOffscreenFramebuffer().GetColorImageView(),
            m_Renderer->GetOffscreenFramebuffer().GetSampler()
        );
    }
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
    } else {
        context.DrawRect(m_Geometry, Color{ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    uint32_t triangleCount = 0;
    uint32_t drawCallCount = 0;
    for (const auto& entity : m_Scene->GetEntities()) {
        drawCallCount++;
        if (entity.Type == we::runtime::scene::EntityType::Plane
            || entity.Type == we::runtime::scene::EntityType::GroundPlane) {
            triangleCount += 2;
        } else {
            triangleCount += 12;
        }
    }

    Color overlayColor = Color{ 0.85f, 0.85f, 0.85f, 1.0f };
    float overlayX = m_Geometry.x + 15.0f;
    float overlayY = m_Geometry.y + 15.0f;

    context.DrawText("FPS: " + std::to_string(static_cast<int>(m_FPS)) + " (" + std::to_string(m_FrameTime).substr(0, 4) + " ms)",
        Point{ overlayX, overlayY }, overlayColor, 12.0f);
    context.DrawText("Triangles: " + std::to_string(triangleCount), Point{ overlayX, overlayY + 16.0f }, overlayColor, 12.0f);
    context.DrawText("Draw Calls: " + std::to_string(drawCallCount), Point{ overlayX, overlayY + 32.0f }, overlayColor, 12.0f);
    context.DrawText("Entities: " + std::to_string(m_Scene->GetEntities().size()), Point{ overlayX, overlayY + 48.0f }, overlayColor, 12.0f);

    glm::vec3 camPos = m_Camera->GetPosition();
    const glm::vec3 prevPos = m_Camera->GetPreviousPosition();
    const float pitch = m_Camera->GetPitch();
    const float yaw = m_Camera->GetYaw();
    const float prevPitch = m_Camera->GetPreviousPitch();
    const float prevYaw = m_Camera->GetPreviousYaw();
    const float frameDt = m_Camera->GetLastDeltaTime();

    auto formatVec3 = [](const glm::vec3& v) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3) << v.x << ", " << v.y << ", " << v.z;
        return ss.str();
    };
    auto formatMat4Row = [](const glm::mat4& m, int row) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3)
           << m[row][0] << ", " << m[row][1] << ", " << m[row][2] << ", " << m[row][3];
        return ss.str();
    };

    float lineY = overlayY + 64.0f;
    const float lineStep = 14.0f;
    const Color debugColor = Color{ 0.78f, 0.92f, 0.78f, 1.0f };

    context.DrawText("Cam Pos: " + formatVec3(camPos), Point{ overlayX, lineY }, overlayColor, 12.0f);
    lineY += lineStep;
    context.DrawText("Cam Rot: pitch " + std::to_string(pitch).substr(0, 6)
        + " yaw " + std::to_string(yaw).substr(0, 6), Point{ overlayX, lineY }, overlayColor, 12.0f);
    lineY += lineStep;
    context.DrawText("Prev Pos: " + formatVec3(prevPos), Point{ overlayX, lineY }, debugColor, 12.0f);
    lineY += lineStep;
    context.DrawText("Prev Rot: pitch " + std::to_string(prevPitch).substr(0, 6)
        + " yaw " + std::to_string(prevYaw).substr(0, 6), Point{ overlayX, lineY }, debugColor, 12.0f);
    lineY += lineStep;
    context.DrawText("Frame dt: " + std::to_string(frameDt * 1000.0f).substr(0, 6) + " ms",
        Point{ overlayX, lineY }, debugColor, 12.0f);
    lineY += lineStep;

    const auto& gpuCam = m_Renderer->GetLastUploadedCameraUniform();
    context.DrawText("GPU View[0]: " + formatMat4Row(gpuCam.view, 0), Point{ overlayX, lineY }, debugColor, 11.0f);
    lineY += lineStep;
    context.DrawText("GPU Proj[0]: " + formatMat4Row(gpuCam.proj, 0), Point{ overlayX, lineY }, debugColor, 11.0f);
    lineY += lineStep;

    const int cameraSpeed = static_cast<int>(std::lround(m_Camera->GetCameraSpeed()));
    context.DrawText("Camera Speed: " + std::to_string(cameraSpeed), Point{ overlayX, lineY }, overlayColor, 12.0f);
    lineY += lineStep;

    const std::string renderStats = we::runtime::renderer::FrameStatsCollector::Get().GetOverlayText();
    context.DrawText(renderStats, Point{ overlayX, lineY }, Color{ 0.65f, 0.82f, 0.95f, 1.0f }, 11.0f);
    lineY += lineStep;
    const std::string diagSummary = we::runtime::renderer::RenderDiagnostics::Get().GetSummary();
    context.DrawText(diagSummary, Point{ overlayX, lineY }, Color{ 0.75f, 0.75f, 0.75f, 1.0f }, 11.0f);

    const std::string renderStats = we::runtime::renderer::FrameStatsCollector::Get().GetOverlayText();
    context.DrawText(renderStats, Point{ overlayX, overlayY + 96.0f }, Color{ 0.65f, 0.82f, 0.95f, 1.0f }, 11.0f);
    const std::string diagSummary = we::runtime::renderer::RenderDiagnostics::Get().GetSummary();
    context.DrawText(diagSummary, Point{ overlayX, overlayY + 112.0f }, Color{ 0.75f, 0.75f, 0.75f, 1.0f }, 11.0f);

    Point gizmoCenter = Point{ m_Geometry.x + m_Geometry.width - 55.0f, m_Geometry.y + 55.0f };
    context.DrawRect(Rect{ gizmoCenter.x - 30.0f, gizmoCenter.y - 30.0f, 60.0f, 60.0f }, Color{ 0.12f, 0.12f, 0.12f, 0.6f }, 30.0f);

    glm::vec3 right = m_Camera->GetRight();
    glm::vec3 up = m_Camera->GetUp();
    glm::vec3 forward = m_Camera->GetForward();

    Color colorX = Color{ 0.9f, 0.25f, 0.2f, 1.0f };
    Color colorY = Color{ 0.25f, 0.9f, 0.2f, 1.0f };
    Color colorZ = Color{ 0.2f, 0.45f, 0.9f, 1.0f };

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
        context.DrawText(axis.label, Point{ endPoint.x + 4.0f, endPoint.y - 6.0f }, Color::White(), 10.0f);
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

    if (m_Geometry.Contains(event.position)
        && we::programs::editor::PlaceActorsPlacement::Get().HasActivePlacement()) {
        const float localX = event.position.x - m_Geometry.x;
        const float localY = event.position.y - m_Geometry.y;
        if (we::programs::editor::PlaceActorsPlacement::Get().TryPlaceAtViewportPoint(localX, localY)) {
            return;
        }
    }

    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        auto& child = *it;
        const Rect geom = child->GetGeometry();
        if (event.position.x >= geom.x && event.position.x <= geom.x + geom.width &&
            event.position.y >= geom.y && event.position.y <= geom.y + geom.height) {
            child->OnMouseDown(event);
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
    m_Navigation.Tick(deltaTime);
}

} // namespace we::UI
