#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Core/Math/Types.h"

#include <span>
#include <string_view>
#include <vector>

namespace we::editor::viewportedit {

class VIEWPORTEDIT_API IViewportCameraController {
public:
    virtual ~IViewportCameraController() = default;

    virtual void SetProjection(CameraProjection projection) = 0;
    [[nodiscard]] virtual CameraProjection GetProjection() const noexcept = 0;

    virtual void FocusSelected(std::span<const ViewportObjectId> ids) = 0;
    virtual void FrameSelection(std::span<const ViewportObjectId> ids) = 0;
    virtual void Orbit(float dx, float dy) = 0;
    virtual void Pan(float dx, float dy) = 0;
    virtual void Zoom(float delta) = 0;
    virtual void FlyLook(float dx, float dy) = 0;

    [[nodiscard]] virtual we::math::Vec3 GetPosition() const = 0;
    [[nodiscard]] virtual we::math::Mat4 GetViewMatrix() const = 0;
    [[nodiscard]] virtual we::math::Mat4 GetProjectionMatrix() const = 0;

    virtual CameraBookmark SaveBookmark(std::string_view name) = 0;
    virtual bool RestoreBookmark(std::string_view name) = 0;
    [[nodiscard]] virtual std::vector<CameraBookmark> ListBookmarks() const = 0;
};

} // namespace we::editor::viewportedit
