#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Core/Math/Types.h"

#include <functional>
#include <memory>
#include <span>
#include <string_view>

namespace we::editor::viewportedit {

/// Transform / selection / delete commands — always recorded through Undo.
class VIEWPORTEDIT_API IViewportCommand {
public:
    virtual ~IViewportCommand() = default;

    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

using ViewportCommandFactory = std::function<std::unique_ptr<IViewportCommand>()>;

class VIEWPORTEDIT_API IViewportOverlay {
public:
    virtual ~IViewportOverlay() = default;
    virtual void Draw(float viewportW, float viewportH) = 0;
};

class VIEWPORTEDIT_API IViewportRenderer {
public:
    virtual ~IViewportRenderer() = default;

    /// Selection outlines / hover / gizmo draw hooks (renderer-agnostic).
    virtual void BeginFrame() = 0;
    virtual void DrawSelection(std::span<const ViewportObjectId> ids) = 0;
    virtual void DrawHover(ViewportObjectId id) = 0;
    virtual void DrawGizmo(const we::math::Mat4& worldMatrix, ViewportToolId tool) = 0;
    virtual void DrawGrid() = 0;
    virtual void EndFrame() = 0;
};

class VIEWPORTEDIT_API IViewportDragDrop {
public:
    virtual ~IViewportDragDrop() = default;

    [[nodiscard]] virtual bool CanDrop(std::string_view payloadType) const = 0;
    virtual void OnDragOver(float x, float y, std::string_view payloadType) = 0;
    virtual bool OnDrop(float x, float y, std::string_view payloadType, std::string_view payloadData) = 0;
};

} // namespace we::editor::viewportedit
