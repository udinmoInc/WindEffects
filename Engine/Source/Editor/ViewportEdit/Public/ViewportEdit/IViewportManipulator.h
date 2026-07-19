#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Core/Math/Types.h"

#include <span>

namespace we::editor::viewportedit {

enum class ManipulatorAxis : std::uint8_t {
    None = 0,
    X,
    Y,
    Z,
    XY,
    XZ,
    YZ,
    XYZ,
};

class VIEWPORTEDIT_API IViewportManipulator {
public:
    virtual ~IViewportManipulator() = default;

    virtual void SetTool(ViewportToolId tool) = 0;
    virtual void SetSpace(TransformSpace space) = 0;
    [[nodiscard]] virtual TransformSpace GetSpace() const noexcept = 0;

    virtual void BeginDrag(ManipulatorAxis axis, float screenX, float screenY) = 0;
    virtual void UpdateDrag(float screenX, float screenY) = 0;
    virtual void EndDrag(bool commit) = 0;
    [[nodiscard]] virtual bool IsDragging() const noexcept = 0;

    /// Apply delta to current selection (records Undo when commit=true on EndDrag).
    virtual void ApplyTranslation(const we::math::Vec3& delta) = 0;
    virtual void ApplyRotationDegrees(const we::math::Vec3& deltaEuler) = 0;
    virtual void ApplyScale(const we::math::Vec3& deltaScale) = 0;
};

} // namespace we::editor::viewportedit
