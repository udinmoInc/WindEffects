#pragma once

#include "Viewport/Export.hpp"

#include <volk.h>
#include "Core/Widget.hpp"
#include "ViewportNavigation.hpp"
#include <memory>

struct SDL_Window;

namespace we::runtime::renderer { class Renderer; }
namespace we::runtime::engine { class EditorCamera; }
namespace we::runtime::scene { class Scene; }

namespace we::UI {

class UIRenderer;
class GraphicsDebuggerPopup;

class VIEWPORT_API ViewportWidget : public Widget {
public:
    ViewportWidget(we::runtime::renderer::Renderer* renderer,
                   const std::shared_ptr<we::runtime::engine::EditorCamera>& camera,
                   const std::shared_ptr<we::runtime::scene::Scene>& scene,
                   UIRenderer* uiRenderer = nullptr);
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

    bool IsFlyLookActive() const { return m_Navigation.IsFlyLookActive(); }

private:
    bool HitTestGizmoReset(const Point& position) const;

    we::runtime::renderer::Renderer* m_Renderer = nullptr;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;
    UIRenderer* m_uiRenderer = nullptr;
    ViewportNavigationController m_Navigation;

    VkDescriptorSet m_ViewportTextureSet = VK_NULL_HANDLE;

    float m_FPS = 0.0f;
    float m_FrameTime = 0.0f;

    uint32_t m_PendingWidth  = 0;
    uint32_t m_PendingHeight = 0;
    bool     m_ResizePending = false;

    std::shared_ptr<GraphicsDebuggerPopup> m_GraphicsDebugger;
};

} // namespace we::UI
