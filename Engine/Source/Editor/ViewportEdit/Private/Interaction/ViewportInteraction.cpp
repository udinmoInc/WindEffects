#include "ViewportEditInternal.h"

namespace we::editor::viewportedit {
namespace detail {

class ViewportRendererImpl final : public IViewportRenderer {
public:
    void BeginFrame() override {}
    void DrawSelection(std::span<const ViewportObjectId>) override {}
    void DrawHover(ViewportObjectId) override {}
    void DrawGizmo(const we::math::Mat4&, ViewportToolId) override {}
    void DrawGrid() override {}
    void EndFrame() override {}
};

class ViewportDragDropImpl final : public IViewportDragDrop {
public:
    explicit ViewportDragDropImpl(IViewportContext& context)
        : m_Context(context)
    {}

    [[nodiscard]] bool CanDrop(std::string_view payloadType) const override {
        return payloadType == "asset" || payloadType == "actor-class" || payloadType == "prefab" ||
               payloadType == "prefab-guid" || payloadType == "terrain-asset";
    }

    void OnDragOver(float, float, std::string_view) override {}

    bool OnDrop(float x, float y, std::string_view payloadType, std::string_view payloadData) override {
        if (!CanDrop(payloadType)) {
            return false;
        }
        const auto hit = m_Context.HitTester().Pick(
            x, y, m_Context.ViewportWidth(), m_Context.ViewportHeight());
        if (m_ExternalHandler) {
            return m_ExternalHandler(x, y, payloadType, payloadData, hit);
        }
        return false;
    }

    void SetExternalDropHandler(ExternalDropHandler handler) override {
        m_ExternalHandler = std::move(handler);
    }

private:
    IViewportContext& m_Context;
    ExternalDropHandler m_ExternalHandler;
};

class ViewportInteractionImpl final : public IViewportInteraction {
public:
    explicit ViewportInteractionImpl(IViewportContext& context)
        : m_Context(context)
    {}

    void SetActiveTool(IViewportTool* tool) { m_ActiveTool = tool; }

    void SetLayers(std::vector<IViewportInteractionLayer*>* layers) { m_Layers = layers; }

    [[nodiscard]] bool HandleMouseDown(const ViewportInputEvent& e) override {
        if (DispatchLayers(&IViewportInteractionLayer::HandleMouseDown, e)) {
            return true;
        }
        return m_ActiveTool && m_ActiveTool->OnMouseDown(m_Context, e);
    }

    [[nodiscard]] bool HandleMouseUp(const ViewportInputEvent& e) override {
        if (DispatchLayers(&IViewportInteractionLayer::HandleMouseUp, e)) {
            return true;
        }
        return m_ActiveTool && m_ActiveTool->OnMouseUp(m_Context, e);
    }

    [[nodiscard]] bool HandleMouseMove(const ViewportInputEvent& e) override {
        if (DispatchLayers(&IViewportInteractionLayer::HandleMouseMove, e)) {
            return true;
        }
        return m_ActiveTool && m_ActiveTool->OnMouseMove(m_Context, e);
    }

    [[nodiscard]] bool HandleKeyDown(const ViewportInputEvent& e) override {
        if (DispatchLayers(&IViewportInteractionLayer::HandleKeyDown, e)) {
            return true;
        }
        if (e.key == we::platform::KeyCode::F) {
            m_Context.Camera().FrameSelection(m_Context.Selection().GetSelected());
            return true;
        }
        if (e.key == we::platform::KeyCode::Escape) {
            m_Context.Selection().Clear();
            m_Context.NotifySelectionChanged();
            return true;
        }
        return m_ActiveTool && m_ActiveTool->OnKeyDown(m_Context, e);
    }

    [[nodiscard]] bool HandleKeyUp(const ViewportInputEvent& e) override {
        if (DispatchLayers(&IViewportInteractionLayer::HandleKeyUp, e)) {
            return true;
        }
        return m_ActiveTool && m_ActiveTool->OnKeyUp(m_Context, e);
    }

    [[nodiscard]] bool HandleScroll(const ViewportInputEvent& e) override {
        if (DispatchLayers(&IViewportInteractionLayer::HandleScroll, e)) {
            return true;
        }
        m_Context.Camera().Zoom(e.scroll);
        return true;
    }

    void Tick(float deltaSeconds) override {
        if (m_ActiveTool) {
            m_ActiveTool->Tick(m_Context, deltaSeconds);
        }
    }

private:
    using LayerHandler = bool (IViewportInteractionLayer::*)(IViewportContext&, const ViewportInputEvent&);

    [[nodiscard]] bool DispatchLayers(LayerHandler handler, const ViewportInputEvent& e) {
        if (!m_Layers) {
            return false;
        }
        for (auto* layer : *m_Layers) {
            if (layer && layer->IsEnabled() && (layer->*handler)(m_Context, e)) {
                return true;
            }
        }
        return false;
    }

    IViewportContext& m_Context;
    IViewportTool* m_ActiveTool = nullptr;
    std::vector<IViewportInteractionLayer*>* m_Layers = nullptr;
};

std::unique_ptr<IViewportRenderer> CreateRenderer() {
    return std::make_unique<ViewportRendererImpl>();
}

std::unique_ptr<IViewportDragDrop> CreateDragDrop(IViewportContext& context) {
    return std::make_unique<ViewportDragDropImpl>(context);
}

std::unique_ptr<IViewportInteraction> CreateInteraction(IViewportContext& context) {
    return std::make_unique<ViewportInteractionImpl>(context);
}

void BindInteractionTool(IViewportInteraction& interaction, IViewportTool* tool) {
    if (auto* impl = dynamic_cast<ViewportInteractionImpl*>(&interaction)) {
        impl->SetActiveTool(tool);
    }
}

void BindInteractionLayers(
    IViewportInteraction& interaction,
    std::vector<IViewportInteractionLayer*>* layers)
{
    if (auto* impl = dynamic_cast<ViewportInteractionImpl*>(&interaction)) {
        impl->SetLayers(layers);
    }
}

} // namespace detail
} // namespace we::editor::viewportedit
