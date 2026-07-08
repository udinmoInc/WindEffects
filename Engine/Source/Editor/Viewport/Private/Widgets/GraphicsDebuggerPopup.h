#pragma once

#include "Core/Widget.h"
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::engine { class EditorCamera; }
namespace we::runtime::renderer { class Renderer; }
namespace we::runtime::scene { class Scene; }

namespace we::UI {

class GraphicsDebuggerPopup : public Widget {
public:
    GraphicsDebuggerPopup(
        ::we::runtime::renderer::Renderer* renderer,
        const std::shared_ptr<::we::runtime::engine::EditorCamera>& camera,
        const std::shared_ptr<::we::runtime::scene::Scene>& scene);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void SetFrameStats(float fps, float frameTimeMs);
    void ArrangeInViewport(const Rect& viewportRect);

private:
    void BuildLines(std::vector<std::string>& outLines) const;
    void LogMoveIfChanged();

    ::we::runtime::renderer::Renderer* m_Renderer = nullptr;
    std::shared_ptr<::we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<::we::runtime::scene::Scene> m_Scene;

    Rect m_HeaderRect;
    Point m_Offset{ 15.0f, 15.0f };
    Point m_DragOffset{};
    Point m_LastLoggedOffset{ -1.0f, -1.0f };
    bool m_Dragging = false;

    float m_FPS = 0.0f;
    float m_FrameTimeMs = 0.0f;
};

} // namespace we::UI
