#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Core/Math/Types.h"

#include <functional>
#include <memory>
#include <span>
#include <string_view>

namespace we::editor::viewportedit {

class IViewportContext;

/// Transform / selection / delete commands — always recorded through Undo.
class VIEWPORTEDIT_API IViewportCommand {
public:
    virtual ~IViewportCommand() = default;

    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

using ViewportCommandFactory = std::function<std::unique_ptr<IViewportCommand>()>;

/// Routes named commands to Undo-backed handlers. Modes register without owning history.
class VIEWPORTEDIT_API IViewportCommandRouter {
public:
    virtual ~IViewportCommandRouter() = default;

    virtual void Register(std::string_view commandId, ViewportCommandFactory factory) = 0;
    virtual void Unregister(std::string_view commandId) = 0;
    [[nodiscard]] virtual bool Has(std::string_view commandId) const noexcept = 0;

    /// Create, execute, and record Undo. Returns false if unknown or execute failed.
    [[nodiscard]] virtual bool Execute(std::string_view commandId) = 0;

    /// Record an already-applied custom op through Undo (brush stroke, spawn, etc.).
    [[nodiscard]] virtual bool RecordCustom(
        std::string_view label,
        std::function<bool()> undoFn,
        std::function<bool()> redoFn) = 0;
};

class VIEWPORTEDIT_API IViewportOverlay {
public:
    virtual ~IViewportOverlay() = default;

    [[nodiscard]] virtual std::string_view GetId() const noexcept { return "Overlay"; }
    [[nodiscard]] virtual ViewportOverlayPass Pass() const noexcept {
        return ViewportOverlayPass::Screen;
    }
    [[nodiscard]] virtual bool IsEnabled() const noexcept { return true; }

    virtual void Draw(IViewportContext& context, float viewportW, float viewportH) = 0;

    /// Legacy signature used by existing callers.
    virtual void Draw(float viewportW, float viewportH) { (void)viewportW; (void)viewportH; }
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

/// GPU / CPU render hooks contributed by modes (brush preview, terrain outlines, etc.).
class VIEWPORTEDIT_API IViewportRenderExtension {
public:
    virtual ~IViewportRenderExtension() = default;

    [[nodiscard]] virtual std::string_view GetId() const noexcept = 0;
    [[nodiscard]] virtual int SortOrder() const noexcept { return 0; }
    [[nodiscard]] virtual bool IsEnabled() const noexcept { return true; }

    virtual void OnBeginFrame(IViewportContext& context) { (void)context; }
    virtual void OnDrawWorld(IViewportContext& context) { (void)context; }
    virtual void OnDrawOverlay(IViewportContext& context) { (void)context; }
    virtual void OnEndFrame(IViewportContext& context) { (void)context; }
};

class VIEWPORTEDIT_API IViewportDragDrop {
public:
    virtual ~IViewportDragDrop() = default;

    [[nodiscard]] virtual bool CanDrop(std::string_view payloadType) const = 0;
    virtual void OnDragOver(float x, float y, std::string_view payloadType) = 0;
    virtual bool OnDrop(float x, float y, std::string_view payloadType, std::string_view payloadData) = 0;

    /// Host/PrefabEditor injects spawn handlers without ViewportEdit depending on Prefab.
    using ExternalDropHandler = std::function<bool(
        float x,
        float y,
        std::string_view payloadType,
        std::string_view payloadData,
        const ViewportHit& hit)>;
    virtual void SetExternalDropHandler(ExternalDropHandler handler) = 0;
};

} // namespace we::editor::viewportedit
