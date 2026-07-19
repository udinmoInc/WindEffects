#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Core/Math/Types.h"

namespace we::editor::viewportedit {

class VIEWPORTEDIT_API IViewportSnapProvider {
public:
    virtual ~IViewportSnapProvider() = default;

    virtual void SetSettings(const SnapSettings& settings) = 0;
    [[nodiscard]] virtual const SnapSettings& GetSettings() const noexcept = 0;

    [[nodiscard]] virtual we::math::Vec3 SnapTranslation(const we::math::Vec3& value) const = 0;
    [[nodiscard]] virtual we::math::Vec3 SnapRotationDegrees(const we::math::Vec3& eulerDegrees) const = 0;
    [[nodiscard]] virtual we::math::Vec3 SnapScale(const we::math::Vec3& scale) const = 0;
};

class VIEWPORTEDIT_API IViewportGridProvider {
public:
    virtual ~IViewportGridProvider() = default;

    virtual void SetVisible(bool visible) = 0;
    [[nodiscard]] virtual bool IsVisible() const noexcept = 0;
    virtual void SetCellSize(float size) = 0;
    [[nodiscard]] virtual float GetCellSize() const noexcept = 0;
};

} // namespace we::editor::viewportedit
