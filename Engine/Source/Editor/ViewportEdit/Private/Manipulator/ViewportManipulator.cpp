#include "ViewportEditInternal.h"

#include "Scene/Entity.h"

#include <cmath>
#include <vector>

namespace we::editor::viewportedit {
namespace detail {
namespace {

struct TransformState {
    ViewportObjectId id{};
    we::math::Vec3 position{};
    we::math::Vec3 rotation{};
    we::math::Vec3 scale{1.f, 1.f, 1.f};
};

std::vector<TransformState> CaptureTransforms(scene::Scene* scene, std::span<const ViewportObjectId> ids) {
    std::vector<TransformState> states;
    if (!scene) {
        return states;
    }
    for (const auto id : ids) {
        if (auto* entity = scene->FindEntityById(id.value)) {
            TransformState s;
            s.id = id;
            s.position = entity->Position;
            s.rotation = entity->Rotation;
            s.scale = entity->Scale;
            states.push_back(s);
        }
    }
    return states;
}

void ApplyTransforms(scene::Scene* scene, const std::vector<TransformState>& states) {
    if (!scene) {
        return;
    }
    for (const auto& s : states) {
        if (auto* entity = scene->FindEntityById(s.id.value)) {
            entity->Position = s.position;
            entity->Rotation = s.rotation;
            entity->Scale = s.scale;
        }
    }
}

bool RecordTransformTransaction(
    IViewportContext& context,
    std::string_view label,
    std::vector<TransformState> before,
    std::vector<TransformState> after)
{
    auto* tx = context.Transactions();
    if (!tx || before.empty()) {
        ApplyTransforms(context.Scene(), after);
        ViewportEditDiagnostics::Get().OnTransform();
        return false;
    }

    ApplyTransforms(context.Scene(), after);
    const bool ok = tx->RecordCustom(
        label,
        undo::TransactionKind::Generic,
        std::string(label),
        [scene = context.Scene(), before]() {
            ApplyTransforms(scene, before);
            return true;
        },
        [scene = context.Scene(), after]() {
            ApplyTransforms(scene, after);
            return true;
        },
        before.size() * sizeof(TransformState));

    if (ok) {
        ViewportEditDiagnostics::Get().OnTransaction();
    }
    ViewportEditDiagnostics::Get().OnTransform();
    return ok;
}

} // namespace

class ViewportManipulatorImpl final : public IViewportManipulator {
public:
    explicit ViewportManipulatorImpl(IViewportContext& context)
        : m_Context(context)
    {}

    void SetTool(ViewportToolId tool) override { m_Tool = tool; }
    void SetSpace(TransformSpace space) override { m_Space = space; }
    [[nodiscard]] TransformSpace GetSpace() const noexcept override { return m_Space; }

    void BeginDrag(ManipulatorAxis axis, float screenX, float screenY) override {
        m_Axis = axis;
        m_Dragging = true;
        m_StartX = screenX;
        m_StartY = screenY;
        m_Before = CaptureTransforms(m_Context.Scene(), m_Context.Selection().GetSelected());
        m_Working = m_Before;
    }

    void UpdateDrag(float screenX, float screenY) override {
        if (!m_Dragging || m_Before.empty()) {
            return;
        }
        const float dx = screenX - m_StartX;
        const float dy = screenY - m_StartY;
        m_Working = m_Before;

        switch (m_Tool) {
        case ViewportToolId::Move: {
            we::math::Vec3 delta{dx * 0.02f, -dy * 0.02f, 0.f};
            if (m_Axis == ManipulatorAxis::X) {
                delta = {delta.x, 0.f, 0.f};
            } else if (m_Axis == ManipulatorAxis::Y) {
                delta = {0.f, delta.y, 0.f};
            } else if (m_Axis == ManipulatorAxis::Z) {
                delta = {0.f, 0.f, delta.x};
            }
            delta = m_Context.Snap().SnapTranslation(delta);
            for (auto& s : m_Working) {
                s.position.x += delta.x;
                s.position.y += delta.y;
                s.position.z += delta.z;
            }
            break;
        }
        case ViewportToolId::Rotate: {
            we::math::Vec3 delta{0.f, dx * 0.25f, 0.f};
            if (m_Axis == ManipulatorAxis::X) {
                delta = {dy * 0.25f, 0.f, 0.f};
            } else if (m_Axis == ManipulatorAxis::Z) {
                delta = {0.f, 0.f, dx * 0.25f};
            }
            delta = m_Context.Snap().SnapRotationDegrees(delta);
            for (auto& s : m_Working) {
                s.rotation.x += delta.x;
                s.rotation.y += delta.y;
                s.rotation.z += delta.z;
            }
            break;
        }
        case ViewportToolId::Scale: {
            const float f = 1.f + (dx - dy) * 0.005f;
            we::math::Vec3 delta{f, f, f};
            if (m_Axis == ManipulatorAxis::X) {
                delta = {f, 1.f, 1.f};
            } else if (m_Axis == ManipulatorAxis::Y) {
                delta = {1.f, f, 1.f};
            } else if (m_Axis == ManipulatorAxis::Z) {
                delta = {1.f, 1.f, f};
            }
            delta = m_Context.Snap().SnapScale(delta);
            for (auto& s : m_Working) {
                s.scale.x *= delta.x;
                s.scale.y *= delta.y;
                s.scale.z *= delta.z;
            }
            break;
        }
        default:
            break;
        }

        ApplyTransforms(m_Context.Scene(), m_Working);
    }

    void EndDrag(bool commit) override {
        if (!m_Dragging) {
            return;
        }
        m_Dragging = false;
        if (commit) {
            const char* label = "Viewport Transform";
            if (m_Tool == ViewportToolId::Move) {
                label = "Move";
            } else if (m_Tool == ViewportToolId::Rotate) {
                label = "Rotate";
            } else if (m_Tool == ViewportToolId::Scale) {
                label = "Scale";
            }
            (void)RecordTransformTransaction(m_Context, label, m_Before, m_Working);
        } else {
            ApplyTransforms(m_Context.Scene(), m_Before);
        }
        m_Before.clear();
        m_Working.clear();
    }

    [[nodiscard]] bool IsDragging() const noexcept override { return m_Dragging; }

    void ApplyTranslation(const we::math::Vec3& delta) override {
        auto before = CaptureTransforms(m_Context.Scene(), m_Context.Selection().GetSelected());
        auto after = before;
        const auto snapped = m_Context.Snap().SnapTranslation(delta);
        for (auto& s : after) {
            s.position.x += snapped.x;
            s.position.y += snapped.y;
            s.position.z += snapped.z;
        }
        (void)RecordTransformTransaction(m_Context, "Move", std::move(before), std::move(after));
    }

    void ApplyRotationDegrees(const we::math::Vec3& deltaEuler) override {
        auto before = CaptureTransforms(m_Context.Scene(), m_Context.Selection().GetSelected());
        auto after = before;
        const auto snapped = m_Context.Snap().SnapRotationDegrees(deltaEuler);
        for (auto& s : after) {
            s.rotation.x += snapped.x;
            s.rotation.y += snapped.y;
            s.rotation.z += snapped.z;
        }
        (void)RecordTransformTransaction(m_Context, "Rotate", std::move(before), std::move(after));
    }

    void ApplyScale(const we::math::Vec3& deltaScale) override {
        auto before = CaptureTransforms(m_Context.Scene(), m_Context.Selection().GetSelected());
        auto after = before;
        const auto snapped = m_Context.Snap().SnapScale(deltaScale);
        for (auto& s : after) {
            s.scale.x *= snapped.x;
            s.scale.y *= snapped.y;
            s.scale.z *= snapped.z;
        }
        (void)RecordTransformTransaction(m_Context, "Scale", std::move(before), std::move(after));
    }

private:
    IViewportContext& m_Context;
    ViewportToolId m_Tool = ViewportToolId::Select;
    TransformSpace m_Space = TransformSpace::World;
    ManipulatorAxis m_Axis = ManipulatorAxis::XYZ;
    bool m_Dragging = false;
    float m_StartX = 0.f;
    float m_StartY = 0.f;
    std::vector<TransformState> m_Before;
    std::vector<TransformState> m_Working;
};

std::unique_ptr<IViewportManipulator> CreateManipulator(IViewportContext& context) {
    return std::make_unique<ViewportManipulatorImpl>(context);
}

} // namespace detail
} // namespace we::editor::viewportedit
