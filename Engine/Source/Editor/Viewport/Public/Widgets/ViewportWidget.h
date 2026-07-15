#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include "Viewport/Export.h"
#include "Core/Widget.h"
#include "ViewportNavigation.h"
#include "Platform/Types.h"

#include <memory>

namespace we::runtime::renderer { class ISceneViewportController; }
namespace we::runtime::engine { class EditorCamera; }
namespace we::runtime::scene { class Scene; }
namespace we::editor::viewport { class ViewportRenderTarget; }

namespace WindEffects::Editor::UI {

class OverlayRenderer;
class GraphicsDebuggerPopup;

class VIEWPORT_API ViewportWidget : public Widget {
public:
    ViewportWidget(::we::runtime::renderer::ISceneViewportController* viewportController,
                   we::rhi::IRHIDevice* device,
                   we::rhi::Format viewportColorFormat,
                   const std::shared_ptr<::we::runtime::engine::EditorCamera>& camera,
                   const std::shared_ptr<::we::runtime::scene::Scene>& scene,
                   OverlayRenderer* uiRenderer = nullptr);
    ~ViewportWidget() override;

    void Construct();

    void SetWindow(we::platform::WindowId window);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;

    void Tick(float deltaTime) override;

    void FlushPendingResize();
    void SyncRendererViewport();

    bool IsFlyLookActive() const { return m_Navigation.IsFlyLookActive(); }
    bool IsViewportNavigating() const { return m_Navigation.IsViewportNavigating(); }

    we::rhi::RHITextureHandle GetViewportColorTexture() const;

private:
    bool HitTestGizmoReset(const Point& position) const;

    ::we::runtime::renderer::ISceneViewportController* m_ViewportController = nullptr;
    we::rhi::IRHIDevice* m_Device = nullptr;
    we::rhi::Format m_ViewportColorFormat = we::rhi::Format::R8G8B8A8_UNORM;
    std::shared_ptr<::we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<::we::runtime::scene::Scene> m_Scene;
    OverlayRenderer* m_uiRenderer = nullptr;
    ViewportNavigationController m_Navigation;

    we::rhi::RHIDescriptorSetHandle m_ViewportTextureSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    we::rhi::RHITextureViewHandle m_BoundViewportView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHISamplerHandle m_BoundViewportSampler = we::rhi::RHISamplerHandle::Invalid;
    std::unique_ptr<we::editor::viewport::ViewportRenderTarget> m_ViewportRenderTarget;

    float m_FPS = 0.0f;
    float m_FrameTime = 0.0f;

    uint32_t m_PendingWidth = 0;
    uint32_t m_PendingHeight = 0;
    bool m_ResizePending = false;

    bool m_HasLastValidBlit = false;
    uint32_t m_LastBlitX = 0;
    uint32_t m_LastBlitY = 0;
    uint32_t m_LastBlitW = 0;
    uint32_t m_LastBlitH = 0;

    std::shared_ptr<GraphicsDebuggerPopup> m_GraphicsDebugger;
};

} // namespace WindEffects::Editor::UI
