#pragma once

#include "Viewport/Export.h"

#include <volk.h>
#include "Core/Widget.h"
#include "ViewportNavigation.h"
#include <memory>

struct SDL_Window;

namespace we::runtime::renderer { class ISceneViewportController; }
namespace we::runtime::renderer { class ResourceManager; }
namespace we::runtime::renderer { class DeviceContext; }
namespace we::runtime::engine { class EditorCamera; }
namespace we::runtime::scene { class Scene; }

namespace we::editor::viewport { class ViewportRenderTarget; }

namespace we::UI {

class OverlayRenderer;
class GraphicsDebuggerPopup;

class VIEWPORT_API ViewportWidget : public Widget {
public:
    ViewportWidget(::we::runtime::renderer::ISceneViewportController* viewportController,
                   ::we::runtime::renderer::DeviceContext* deviceContext,
                   ::we::runtime::renderer::ResourceManager* resourceManager,
                   VkFormat viewportColorFormat,
                   const std::shared_ptr<::we::runtime::engine::EditorCamera>& camera,
                   const std::shared_ptr<::we::runtime::scene::Scene>& scene,
                   OverlayRenderer* uiRenderer = nullptr);
    virtual ~ViewportWidget();

    // Must be called after the widget is owned by std::shared_ptr (AddChild uses shared_from_this).
    void Construct();

    void SetWindow(SDL_Window* window);

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

    // Exposes the owned offscreen color target for compositor sampling.
    VkImageView GetViewportColorImageView() const;

private:
    bool HitTestGizmoReset(const Point& position) const;

    ::we::runtime::renderer::ISceneViewportController* m_ViewportController = nullptr;
    std::shared_ptr<::we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<::we::runtime::scene::Scene> m_Scene;
    OverlayRenderer* m_uiRenderer = nullptr;
    ViewportNavigationController m_Navigation;

    VkDescriptorSet m_ViewportTextureSet = VK_NULL_HANDLE;

    ::we::runtime::renderer::ResourceManager* m_ResourceManager = nullptr;
    ::we::runtime::renderer::DeviceContext* m_DeviceContext = nullptr;
    VkFormat m_ViewportColorFormat = VK_FORMAT_UNDEFINED;
    std::unique_ptr<we::editor::viewport::ViewportRenderTarget> m_ViewportRenderTarget;

    float m_FPS = 0.0f;
    float m_FrameTime = 0.0f;

    uint32_t m_PendingWidth  = 0;
    uint32_t m_PendingHeight = 0;
    bool     m_ResizePending = false;

    std::shared_ptr<GraphicsDebuggerPopup> m_GraphicsDebugger;
};

} // namespace we::UI
