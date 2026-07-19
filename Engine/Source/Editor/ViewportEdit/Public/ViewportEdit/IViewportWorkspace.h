#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "ViewportEdit/IViewportModeManager.h"
#include "ViewportEdit/IViewportCommand.h"
#include "ViewportEdit/IViewportInteractionLayer.h"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::viewportedit {

class IViewportEditor;
class IViewportContext;
class IViewportModeManager;
class IViewportCommandRouter;
class IViewportSelectionContext;
class IViewportTool;

/// Permanent editing workspace — hosts modes, overlays, extensions, and command routing.
/// Modes load/unload dynamically; core viewport code stays mode-agnostic.
class VIEWPORTEDIT_API IViewportWorkspace {
public:
    virtual ~IViewportWorkspace() = default;

    [[nodiscard]] virtual IViewportEditor& Editor() noexcept = 0;
    [[nodiscard]] virtual IViewportContext& Context() noexcept = 0;
    [[nodiscard]] virtual IViewportModeManager& Modes() noexcept = 0;
    [[nodiscard]] virtual IViewportCommandRouter& Commands() noexcept = 0;
    [[nodiscard]] virtual IViewportSelectionContext& SelectionContext() noexcept = 0;

    virtual void RegisterOverlay(std::unique_ptr<IViewportOverlay> overlay) = 0;
    virtual void UnregisterOverlay(std::string_view overlayId) = 0;
    [[nodiscard]] virtual std::span<const std::unique_ptr<IViewportOverlay>> Overlays() const noexcept = 0;

    virtual void RegisterRenderExtension(std::unique_ptr<IViewportRenderExtension> extension) = 0;
    virtual void UnregisterRenderExtension(std::string_view extensionId) = 0;

    virtual void PushInteractionLayer(std::unique_ptr<IViewportInteractionLayer> layer) = 0;
    virtual void PopInteractionLayer(std::string_view layerId) = 0;

    virtual void RegisterTool(std::unique_ptr<IViewportTool> tool) = 0;
    [[nodiscard]] virtual IViewportTool* FindTool(ViewportToolId id) noexcept = 0;
    [[nodiscard]] virtual IViewportTool* FindTool(std::string_view stableId) noexcept = 0;

    virtual void Tick(float deltaSeconds) = 0;
};

} // namespace we::editor::viewportedit
