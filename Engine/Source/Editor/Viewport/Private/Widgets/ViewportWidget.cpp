#include "Widgets/ViewportWidget.h"
#include "Widgets/GraphicsDebuggerPopup.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "Renderer/Renderer.h"
#include "RHI/Types.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"
#include "Core/PaintContext.h"
#include "Rendering/OverlayRenderer.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Rendering/ViewportRenderTarget.h"
#include "Core/LogCategory.h"
#include "Core/DiagnosticMacros.h"
#include <algorithm>
#include <glm/glm.hpp>

#ifndef WE_DEBUG_UI
#define WE_DEBUG_UI 0
#endif

namespace WindEffects::Editor::UI {

ViewportWidget::ViewportWidget(::we::runtime::renderer::ISceneViewportController* viewportController,
                               we::rhi::IRHIDevice* device,
                               we::rhi::Format viewportColorFormat,
                               const std::shared_ptr<::we::runtime::engine::EditorCamera>& camera,
                               const std::shared_ptr<::we::runtime::scene::Scene>& scene,
                               OverlayRenderer* uiRenderer)
    : m_ViewportController(viewportController)
    , m_Device(device)
    , m_ViewportColorFormat(viewportColorFormat)
    , m_Camera(camera)
    , m_Scene(scene)
    , m_uiRenderer(uiRenderer)
{
    m_Navigation.SetCamera(camera);
    m_Navigation.SetScene(scene);
    m_Navigation.ApplySettingsFromStore();

    m_ViewportRenderTarget = std::make_unique<we::editor::viewport::ViewportRenderTarget>();
    m_ViewportRenderTarget->Init(m_Device, m_ViewportColorFormat);

#if WE_DEBUG_UI
    m_GraphicsDebugger = std::make_shared<GraphicsDebuggerPopup>(nullptr, camera, scene);
#endif
}

void ViewportWidget::Construct() {
#if WE_DEBUG_UI
    AddChild(m_GraphicsDebugger);
    if (m_GraphicsDebugger) {
        m_GraphicsDebugger->SetVisible(false);
    }
#endif
}

ViewportWidget::~ViewportWidget() {
    if (m_uiRenderer && m_ViewportTextureSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_uiRenderer->UnregisterTexture(m_ViewportTextureSet);
        m_ViewportTextureSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    }
}

void ViewportWidget::SetWindow(we::platform::WindowId window) {
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
        m_PendingWidth = static_cast<uint32_t>(allottedRect.width);
        m_PendingHeight = static_cast<uint32_t>(allottedRect.height);
        m_ResizePending = true;
    } else if (allottedRect.width <= 0.0f || allottedRect.height <= 0.0f) {
        m_ResizePending = false;
    }

    for (auto& child : m_Children) {
        if (child == m_GraphicsDebugger) {
            continue;
        }
        float compactHeight = 34.0f;
        child->Measure(Size{allottedRect.width, compactHeight});
        child->Arrange(Rect{allottedRect.x, allottedRect.y, allottedRect.width, compactHeight});
    }

    if (m_GraphicsDebugger && m_GraphicsDebugger->IsVisible()) {
        m_GraphicsDebugger->ArrangeInViewport(allottedRect);
    }
}

void ViewportWidget::FlushPendingResize() {
    if (!m_ResizePending) {
        return;
    }
    m_ResizePending = false;
    if (m_PendingWidth == 0 || m_PendingHeight == 0) {
        return;
    }

    m_Camera->SetViewportSize(static_cast<float>(m_PendingWidth), static_cast<float>(m_PendingHeight));

    if (m_ViewportController) {
        if (m_ViewportRenderTarget) {
            m_ViewportRenderTarget->Resize(m_PendingWidth, m_PendingHeight);
            m_ViewportController->SetViewportRenderTargetColor(m_ViewportRenderTarget->GetColorTexture());
            m_ViewportController->SetViewportDepthTarget(m_ViewportRenderTarget->GetDepthTexture());
        }
        m_ViewportController->SetViewportRenderTargetSize(m_PendingWidth, m_PendingHeight);
    }

    if (m_uiRenderer && m_ViewportController) {
        const auto view = m_ViewportController->GetViewportColorView();
        const auto sampler = m_ViewportController->GetViewportColorSampler();
        if (view != we::rhi::RHITextureViewHandle::Invalid
            && sampler != we::rhi::RHISamplerHandle::Invalid) {
            if (m_ViewportTextureSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
                m_ViewportTextureSet = m_uiRenderer->RegisterTexture(view, sampler);
            } else {
                m_uiRenderer->UpdateTexture(m_ViewportTextureSet, view, sampler);
            }
        }
    }

    SyncRendererViewport();
}

void ViewportWidget::SyncRendererViewport() {
    if (!m_ViewportController) {
        return;
    }

    if (m_Visible && m_Geometry.width > 0.0f && m_Geometry.height > 0.0f) {
        m_LastBlitX = static_cast<uint32_t>(m_Geometry.x);
        m_LastBlitY = static_cast<uint32_t>(m_Geometry.y);
        m_LastBlitW = static_cast<uint32_t>(m_Geometry.width);
        m_LastBlitH = static_cast<uint32_t>(m_Geometry.height);
        m_HasLastValidBlit = true;
        m_ViewportController->SetViewportBlitRect(m_LastBlitX, m_LastBlitY, m_LastBlitW, m_LastBlitH);
        return;
    }

    if (m_HasLastValidBlit) {
        m_ViewportController->SetViewportBlitRect(m_LastBlitX, m_LastBlitY, m_LastBlitW, m_LastBlitH);
        return;
    }

    m_ViewportController->SetViewportBlitRect(0, 0, 0, 0);
}

we::rhi::RHITextureHandle ViewportWidget::GetViewportColorTexture() const {
    return m_ViewportRenderTarget ? m_ViewportRenderTarget->GetColorTexture()
                                  : we::rhi::RHITextureHandle::Invalid;
}

bool ViewportWidget::HitTestGizmoReset(const Point& position) const {
    Point gizmoCenter = Point{m_Geometry.x + m_Geometry.width - 55.0f, m_Geometry.y + 55.0f};
    float dx = position.x - gizmoCenter.x;
    float dy = position.y - gizmoCenter.y;
    return dx * dx + dy * dy <= 900.0f;
}

void ViewportWidget::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    // Bind/update viewport RT after the scene backend creates/resizes the offscreen image.
    if (m_uiRenderer && m_ViewportController) {
        const auto view = m_ViewportController->GetViewportColorView();
        const auto sampler = m_ViewportController->GetViewportColorSampler();
        if (view != we::rhi::RHITextureViewHandle::Invalid
            && sampler != we::rhi::RHISamplerHandle::Invalid) {
            if (m_ViewportTextureSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
                m_ViewportTextureSet = m_uiRenderer->RegisterTexture(view, sampler);
            } else {
                m_uiRenderer->UpdateTexture(m_ViewportTextureSet, view, sampler);
            }
        }
    }

    if (m_ViewportTextureSet != we::rhi::RHIDescriptorSetHandle::Invalid
        && m_Geometry.width > 0.0f
        && m_Geometry.height > 0.0f) {
        context.DrawColorTexture(m_Geometry, m_ViewportTextureSet);
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
        && m_Geometry.Contains(event.position)) {
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
#if WE_DEBUG_UI
    if (event.type == KeyEventType::KeyDown && event.key == we::platform::KeyCode::F8 && m_GraphicsDebugger) {
        m_GraphicsDebugger->SetVisible(!m_GraphicsDebugger->IsVisible());
        return;
    }
#endif
    m_Navigation.OnKeyDown(event);
}

void ViewportWidget::Tick(float deltaTime) {
    m_FPS = 1.0f / (deltaTime > 0.0f ? deltaTime : 0.016f);
    m_FrameTime = deltaTime * 1000.0f;
    if (m_GraphicsDebugger && m_GraphicsDebugger->IsVisible()) {
        m_GraphicsDebugger->SetFrameStats(m_FPS, m_FrameTime);
    }
    m_Navigation.Tick(deltaTime);
}

} // namespace WindEffects::Editor::UI
