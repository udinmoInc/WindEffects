#include "ViewportEditInternal.h"

namespace we::editor::viewportedit {
namespace detail {
namespace {

class ToolBase : public IViewportTool {
public:
    void OnActivated(IViewportContext& context) override {
        (void)context;
        ViewportEditDiagnostics::Get().OnToolActivated();
    }
    void OnDeactivated(IViewportContext& context) override { (void)context; }
    void Tick(IViewportContext& context, float) override { (void)context; }
    [[nodiscard]] bool OnKeyDown(IViewportContext&, const ViewportInputEvent&) override { return false; }
    [[nodiscard]] bool OnKeyUp(IViewportContext&, const ViewportInputEvent&) override { return false; }
};

class SelectTool final : public ToolBase {
public:
    [[nodiscard]] ViewportToolId GetId() const noexcept override { return ViewportToolId::Select; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Select"; }

    [[nodiscard]] bool OnMouseDown(IViewportContext& context, const ViewportInputEvent& e) override {
        if (e.button != ViewportMouseButton::Left) {
            return false;
        }
        // Alt+LMB is reserved for viewport orbit navigation.
        if (e.alt) {
            return false;
        }

        m_Down = true;
        m_DownX = e.x;
        m_DownY = e.y;
        m_Marquee = false;
        m_Moved = false;
        return true;
    }

    [[nodiscard]] bool OnMouseUp(IViewportContext& context, const ViewportInputEvent& e) override {
        if (!m_Down || e.button != ViewportMouseButton::Left) {
            return false;
        }
        m_Down = false;

        if (m_Marquee) {
            ViewportRect rect = m_Rect;
            if (rect.width < 0.f) {
                rect.x += rect.width;
                rect.width = -rect.width;
            }
            if (rect.height < 0.f) {
                rect.y += rect.height;
                rect.height = -rect.height;
            }
            const auto ids = context.HitTester().QueryBox(
                rect, context.ViewportWidth(), context.ViewportHeight());
            SelectionOp op = e.shift ? SelectionOp::Add : SelectionOp::Replace;
            if (ids.empty() && op == SelectionOp::Replace) {
                context.Selection().Clear();
            } else {
                context.Selection().ApplyMany(op, ids);
            }
            context.NotifySelectionChanged();
            m_Marquee = false;
            return true;
        }

        if (m_Moved) {
            return true;
        }

        const auto hit = context.HitTester().Pick(
            e.x, e.y, context.ViewportWidth(), context.ViewportHeight());
        SelectionOp op = SelectionOp::Replace;
        if (e.ctrl || e.shift) {
            op = SelectionOp::Toggle;
        }
        if (hit.valid) {
            context.Selection().Apply(op, hit.object);
        } else if (op == SelectionOp::Replace) {
            context.Selection().Clear();
        }
        context.NotifySelectionChanged();
        return true;
    }

    [[nodiscard]] bool OnMouseMove(IViewportContext& context, const ViewportInputEvent& e) override {
        if (m_Down) {
            const float dx = e.x - m_DownX;
            const float dy = e.y - m_DownY;
            if (!m_Marquee && (dx * dx + dy * dy) > 16.f) {
                m_Marquee = true;
                m_Rect = ViewportRect{m_DownX, m_DownY, 0.f, 0.f};
            }
            if (m_Marquee) {
                m_Moved = true;
                m_Rect.width = e.x - m_Rect.x;
                m_Rect.height = e.y - m_Rect.y;
                return true;
            }
        }
        const auto hit = context.HitTester().Pick(
            e.x, e.y, context.ViewportWidth(), context.ViewportHeight());
        context.Selection().SetHovered(hit.valid ? hit.object : ViewportObjectId{});
        return false;
    }

private:
    bool m_Down = false;
    bool m_Marquee = false;
    bool m_Moved = false;
    float m_DownX = 0.f;
    float m_DownY = 0.f;
    ViewportRect m_Rect{};
};

class TransformToolBase : public ToolBase {
public:
    explicit TransformToolBase(ViewportToolId id, std::string_view name)
        : m_Id(id)
        , m_Name(name)
    {}

    [[nodiscard]] ViewportToolId GetId() const noexcept override { return m_Id; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return m_Name; }

    void OnActivated(IViewportContext& context) override {
        ToolBase::OnActivated(context);
        context.Manipulator().SetTool(m_Id);
    }

    [[nodiscard]] bool OnMouseDown(IViewportContext& context, const ViewportInputEvent& e) override {
        if (e.button != ViewportMouseButton::Left || e.alt) {
            return false;
        }
        if (context.Selection().IsEmpty()) {
            // Fall back to select-on-click when nothing selected.
            const auto hit = context.HitTester().Pick(
                e.x, e.y, context.ViewportWidth(), context.ViewportHeight());
            if (hit.valid) {
                context.Selection().Set(hit.object);
                context.NotifySelectionChanged();
            }
            return hit.valid;
        }
        context.Manipulator().BeginDrag(ManipulatorAxis::XYZ, e.x, e.y);
        m_Dragging = true;
        return true;
    }

    [[nodiscard]] bool OnMouseMove(IViewportContext& context, const ViewportInputEvent& e) override {
        if (!m_Dragging) {
            return false;
        }
        context.Manipulator().UpdateDrag(e.x, e.y);
        return true;
    }

    [[nodiscard]] bool OnMouseUp(IViewportContext& context, const ViewportInputEvent& e) override {
        if (!m_Dragging || e.button != ViewportMouseButton::Left) {
            return false;
        }
        m_Dragging = false;
        context.Manipulator().EndDrag(true);
        return true;
    }

private:
    ViewportToolId m_Id;
    std::string_view m_Name;
    bool m_Dragging = false;
};

class DefaultMode final : public IViewportMode {
public:
    [[nodiscard]] ViewportModeId GetId() const noexcept override { return ViewportModeId::Default; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Default"; }
    void OnEnter(IViewportContext&) override {}
    void OnExit(IViewportContext&) override {}
    void Tick(IViewportContext&, float) override {}
};

} // namespace

std::unique_ptr<IViewportTool> CreateSelectTool() { return std::make_unique<SelectTool>(); }
std::unique_ptr<IViewportTool> CreateMoveTool() {
    return std::make_unique<TransformToolBase>(ViewportToolId::Move, "Move");
}
std::unique_ptr<IViewportTool> CreateRotateTool() {
    return std::make_unique<TransformToolBase>(ViewportToolId::Rotate, "Rotate");
}
std::unique_ptr<IViewportTool> CreateScaleTool() {
    return std::make_unique<TransformToolBase>(ViewportToolId::Scale, "Scale");
}
std::unique_ptr<IViewportMode> CreateDefaultMode() { return std::make_unique<DefaultMode>(); }

} // namespace detail
} // namespace we::editor::viewportedit
