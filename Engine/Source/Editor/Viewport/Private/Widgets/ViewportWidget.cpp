#include "Widgets/ViewportWidget.hpp"
#include "Widgets/ToolsPanel.hpp"
#include "Renderer/Renderer.hpp"
#include "EditorCamera.hpp"
#include "Scene/Scene.hpp"
#include "Core/PaintContext.hpp"
#include "Rendering/UIRenderer.hpp"
#include "Core/Theme.hpp"
#include <SDL3/SDL_keyboard.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace we::UI {

ViewportWidget::ViewportWidget(const std::shared_ptr<we::runtime::renderer::Renderer>& renderer,
                               const std::shared_ptr<we::runtime::engine::EditorCamera>& camera,
                               const std::shared_ptr<we::runtime::scene::Scene>& scene,
                               UIRenderer* uiRenderer)
    : m_Renderer(renderer), m_Camera(camera), m_Scene(scene), m_uiRenderer(uiRenderer) {}

ViewportWidget::~ViewportWidget() {
    if (m_uiRenderer && m_ViewportTextureSet != VK_NULL_HANDLE) {
        m_uiRenderer->UnregisterTexture(m_ViewportTextureSet);
        m_ViewportTextureSet = VK_NULL_HANDLE;
    }
}

Size ViewportWidget::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void ViewportWidget::Arrange(const Rect& allottedRect) {
    bool sizeChanged = allottedRect.width != m_Geometry.width || allottedRect.height != m_Geometry.height;
    m_Geometry = allottedRect;

    if (sizeChanged && allottedRect.width > 0.0f && allottedRect.height > 0.0f) {
        // Do NOT touch GPU resources here — we are inside the layout/paint pass
        // which runs mid-frame inside UIRenderer::Render(). Record the new size
        // and let FlushPendingResize() apply it safely before the next BeginFrame().
        m_PendingWidth  = static_cast<uint32_t>(allottedRect.width);
        m_PendingHeight = static_cast<uint32_t>(allottedRect.height);
        m_ResizePending = true;
    }

  if (m_ToolsPanel) {
        if (auto* toolsPanel = dynamic_cast<we::programs::editor::ToolsPanel*>(m_ToolsPanel.get())) {
            m_ToolsPanelWidth = toolsPanel->GetOccupiedWidth();
            m_ToolsPanel->Measure(Size{ m_ToolsPanelWidth, allottedRect.height });
            m_ToolsPanel->Arrange(Rect{ allottedRect.x, allottedRect.y, m_ToolsPanelWidth, allottedRect.height });
        }
    }

    // Arrange integrated compact toolbar at top (assuming it's the first child)
    if (!m_Children.empty()) {
        auto toolbar = m_Children[0];
        float compactHeight = 34.0f;
        toolbar->Measure(Size{ allottedRect.width, compactHeight });
        toolbar->Arrange(Rect{ allottedRect.x, allottedRect.y, allottedRect.width, compactHeight });
    }
}

void ViewportWidget::SetToolsPanel(const std::shared_ptr<Widget>& toolsPanel) {
    m_ToolsPanel = toolsPanel;
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

    // Re-register the viewport texture with the new image view
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

void ViewportWidget::Paint(PaintContext& context) {
    if (!m_Visible) return;

    // 1. Draw 3D scene viewport texture
    if (m_ViewportTextureSet != VK_NULL_HANDLE) {
        context.DrawTexture(m_Geometry, m_ViewportTextureSet);
    } else {
        // Vertical gradient: Top #232427, Bottom #17181A
        Color gradTop{0.137f, 0.141f, 0.153f, 1.0f};
        Color gradBottom{0.090f, 0.094f, 0.102f, 1.0f};
        context.DrawGradient(m_Geometry, gradTop, gradBottom);
    }

    // 2. Draw Stats Overlay (Top-Left)
    uint32_t triangleCount = 0;
    uint32_t drawCallCount = 0;
    for (const auto& entity : m_Scene->GetEntities()) {
        drawCallCount++;
        if (entity.Type == we::runtime::scene::EntityType::Plane) {
            triangleCount += 2;
        } else {
            triangleCount += 12; // Cube
        }
    }

    Color overlayColor = Color{ 0.85f, 0.85f, 0.85f, 1.0f };
    float overlayX = m_Geometry.x + m_ToolsPanelWidth + 15.0f;
    float overlayY = m_Geometry.y + 15.0f;

    context.DrawText("FPS: " + std::to_string(static_cast<int>(m_FPS)) + " (" + std::to_string(m_FrameTime).substr(0, 4) + " ms)", Point{ overlayX, overlayY }, overlayColor, 12.0f);
    context.DrawText("Triangles: " + std::to_string(triangleCount), Point{ overlayX, overlayY + 16.0f }, overlayColor, 12.0f);
    context.DrawText("Draw Calls: " + std::to_string(drawCallCount), Point{ overlayX, overlayY + 32.0f }, overlayColor, 12.0f);
    context.DrawText("Entities: " + std::to_string(m_Scene->GetEntities().size()), Point{ overlayX, overlayY + 48.0f }, overlayColor, 12.0f);

    glm::vec3 camPos = m_Camera->GetPosition();
    std::string posStr = "Cam Pos: " + std::to_string(camPos.x).substr(0, 4) + ", " + std::to_string(camPos.y).substr(0, 4) + ", " + std::to_string(camPos.z).substr(0, 4);
    context.DrawText(posStr, Point{ overlayX, overlayY + 64.0f }, overlayColor, 12.0f);

    // 3. Draw Clickable 3D Axis Gizmo (Top-Right)
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
    
    // 4. Paint tools drawer overlay (left)
    if (m_ToolsPanel) {
        m_ToolsPanel->Paint(context);
    }

    // 5. Paint children (e.g., floating toolbar)
    for (auto& child : m_Children) {
        child->Paint(context);
    }
}

void ViewportWidget::OnMouseDown(const MouseEvent& event) {
    m_LastMousePos = event.position;

    if (m_ToolsPanel) {
        if (auto* toolsPanel = dynamic_cast<we::programs::editor::ToolsPanel*>(m_ToolsPanel.get())) {
            if (toolsPanel->HitTest(event.position)) {
                m_ToolsPanel->OnMouseDown(event);
                return;
            }
        }
    }

    if (event.button == MouseButton::Right) m_RightMouseDown = true;
    if (event.button == MouseButton::Left) m_LeftMouseDown = true;
    if (event.button == MouseButton::Middle) m_MiddleMouseDown = true;

    Point gizmoCenter = Point{ m_Geometry.x + m_Geometry.width - 55.0f, m_Geometry.y + 55.0f };
    float dx = event.position.x - gizmoCenter.x;
    float dy = event.position.y - gizmoCenter.y;
    if (dx * dx + dy * dy <= 900.0f) {
        m_Camera->Reset();
        return; // Intercepted
    }
    
    // Check children first
    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        auto& child = *it;
        if (event.position.x >= child->GetGeometry().x && event.position.x <= child->GetGeometry().x + child->GetGeometry().width &&
            event.position.y >= child->GetGeometry().y && event.position.y <= child->GetGeometry().y + child->GetGeometry().height) {
            child->OnMouseDown(event);
            return; // Intercepted
        }
    }
}

void ViewportWidget::OnMouseMove(const MouseEvent& event) {
    if (m_ToolsPanel) {
        m_ToolsPanel->OnMouseMove(event);
    }

    if (!m_Focused && !m_LeftMouseDown && !m_RightMouseDown && !m_MiddleMouseDown) {
        return;
    }

    float dx = event.position.x - m_LastMousePos.x;
    float dy = event.position.y - m_LastMousePos.y;

    if (event.altDown) {
        if (m_LeftMouseDown) {
            m_Camera->ProcessMouseOrbit(dx, dy);
        } else if (m_MiddleMouseDown) {
            m_Camera->ProcessMousePan(dx, dy);
        } else if (m_RightMouseDown) {
            m_Camera->ProcessMouseScroll(dx * 0.1f);
        }
    } else {
        if (m_RightMouseDown) {
            m_Camera->SetFPSMode(true);
            m_Camera->ProcessMouseFPS(dx, dy);
        } else {
            m_Camera->SetFPSMode(false);
        }
    }

    m_LastMousePos = event.position;

    // Route to children
    for (auto& child : m_Children) {
        child->OnMouseMove(event);
    }
}

void ViewportWidget::OnMouseUp(const MouseEvent& event) {
    if (m_ToolsPanel) {
        m_ToolsPanel->OnMouseUp(event);
    }

    if (event.button == MouseButton::Right) {
        m_RightMouseDown = false;
        m_Camera->SetFPSMode(false);
    }
    if (event.button == MouseButton::Left) m_LeftMouseDown = false;
    if (event.button == MouseButton::Middle) m_MiddleMouseDown = false;

    // Route to children
    for (auto& child : m_Children) {
        child->OnMouseUp(event);
    }
}

void ViewportWidget::OnMouseWheel(const MouseEvent& event) {
    m_Camera->ProcessMouseScroll(event.wheelDeltaY * 0.5f);
}

void ViewportWidget::Tick(float deltaTime) {
    if (m_ToolsPanel) {
        m_ToolsPanel->Tick(deltaTime);
        if (auto* toolsPanel = dynamic_cast<we::programs::editor::ToolsPanel*>(m_ToolsPanel.get())) {
            float occupiedWidth = toolsPanel->GetOccupiedWidth();
            if (std::abs(m_ToolsPanelWidth - occupiedWidth) > 0.5f) {
                m_ToolsPanelWidth = occupiedWidth;
                Arrange(m_Geometry); // Re-layout the Viewport
            }
        }
    }

    m_FPS = 1.0f / (deltaTime > 0.0f ? deltaTime : 0.016f);
    m_FrameTime = deltaTime * 1000.0f;

    if (m_RightMouseDown) {
        const bool* state = SDL_GetKeyboardState(nullptr);
        bool keys[512] = { false };
        keys[SDL_SCANCODE_W] = state[SDL_SCANCODE_W];
        keys[SDL_SCANCODE_S] = state[SDL_SCANCODE_S];
        keys[SDL_SCANCODE_A] = state[SDL_SCANCODE_A];
        keys[SDL_SCANCODE_D] = state[SDL_SCANCODE_D];
        keys[SDL_SCANCODE_E] = state[SDL_SCANCODE_E];
        keys[SDL_SCANCODE_Q] = state[SDL_SCANCODE_Q];
        keys[SDL_SCANCODE_LSHIFT] = state[SDL_SCANCODE_LSHIFT];

        m_Camera->ProcessKeyboard(keys, deltaTime);
    }
}

} // namespace we::editor::viewport::UI
