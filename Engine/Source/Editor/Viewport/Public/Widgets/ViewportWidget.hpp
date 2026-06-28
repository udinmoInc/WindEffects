#pragma once

#include <volk.h>
#include "Core/Widget.hpp"
#include <memory>

namespace we::runtime::renderer { class Renderer; }
namespace we::runtime::engine { class EditorCamera; }
namespace we::runtime::scene { class Scene; }

namespace we::UI {

class UIRenderer;

class ViewportWidget : public Widget {
public:
    ViewportWidget(const std::shared_ptr<we::runtime::renderer::Renderer>& renderer,
                   const std::shared_ptr<we::runtime::engine::EditorCamera>& camera,
                   const std::shared_ptr<we::runtime::scene::Scene>& scene,
                   UIRenderer* uiRenderer = nullptr);
    virtual ~ViewportWidget();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;

    void Tick(float deltaTime) override;

    // Call once per frame BEFORE BeginFrame to apply any pending resize
    // without touching GPU resources mid command-buffer recording.
    void FlushPendingResize();

private:
    std::shared_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;
    UIRenderer* m_uiRenderer = nullptr;

    VkDescriptorSet m_ViewportTextureSet = VK_NULL_HANDLE;

    // Mouse tracking
    Point m_LastMousePos;
    bool m_RightMouseDown = false;
    bool m_LeftMouseDown = false;
    bool m_MiddleMouseDown = false;

    float m_FPS = 0.0f;
    float m_FrameTime = 0.0f;

    // Deferred resize: set in Arrange(), applied in FlushPendingResize()
    uint32_t m_PendingWidth  = 0;
    uint32_t m_PendingHeight = 0;
    bool     m_ResizePending = false;
};

} // namespace we::editor::viewport::UI
