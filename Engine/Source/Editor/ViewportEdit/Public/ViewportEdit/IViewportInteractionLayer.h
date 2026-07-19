#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "ViewportEdit/IViewportTool.h"

#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::viewportedit {

class IViewportContext;
class IViewportSelection;

/// Selection semantics for the active mode (filters, terrain-only, actor-only, etc.).
class VIEWPORTEDIT_API IViewportSelectionContext {
public:
    virtual ~IViewportSelectionContext() = default;

    [[nodiscard]] virtual IViewportSelection& Selection() noexcept = 0;
    [[nodiscard]] virtual const IViewportSelection& Selection() const noexcept = 0;

    /// Mode may restrict which objects are pickable.
    [[nodiscard]] virtual bool IsSelectable(ViewportObjectId id) const noexcept = 0;
    virtual void SetFilter(std::function<bool(ViewportObjectId)> filter) = 0;
    virtual void ClearFilter() = 0;

    /// Terrain / foliage / water may attach typed selection payloads later.
    virtual void SetPrimaryTerrain(ViewportObjectId id) = 0;
    [[nodiscard]] virtual ViewportObjectId PrimaryTerrain() const noexcept = 0;
};

/// Prioritized input layer — modes/tools/overlays stack without fighting the core router.
class VIEWPORTEDIT_API IViewportInteractionLayer {
public:
    virtual ~IViewportInteractionLayer() = default;

    [[nodiscard]] virtual std::string_view GetId() const noexcept = 0;
    [[nodiscard]] virtual ViewportInteractionPriority Priority() const noexcept = 0;
    [[nodiscard]] virtual bool IsEnabled() const noexcept { return true; }

    [[nodiscard]] virtual bool HandleMouseDown(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleMouseUp(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleMouseMove(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleKeyDown(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleKeyUp(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleScroll(IViewportContext& context, const ViewportInputEvent& e) = 0;
};

} // namespace we::editor::viewportedit
