#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "ViewportEdit/IViewportTool.h"

#include <memory>
#include <string_view>

namespace we::editor::undo {
class IUndoRuntime;
class ITransactionManager;
}

namespace we::editor::property {
class IPropertyEditorRuntime;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::world {
class IWorldRuntime;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::editor::viewportedit {

class IViewportSelection;
class IViewportHitTester;
class IViewportCameraController;
class IViewportManipulator;
class IViewportSnapProvider;
class IViewportGridProvider;
class IViewportOverlay;
class IViewportRenderer;
class IViewportDragDrop;
class IViewportModeManager;
class IViewportWorkspace;
class IViewportCommandRouter;
class IViewportSelectionContext;
class IViewportToolContext;

/// Shared mutable editing context — tools/modes/manipulators consume this; never owns gameplay data.
class VIEWPORTEDIT_API IViewportContext {
public:
    virtual ~IViewportContext() = default;

    [[nodiscard]] virtual IViewportSelection& Selection() noexcept = 0;
    [[nodiscard]] virtual IViewportHitTester& HitTester() noexcept = 0;
    [[nodiscard]] virtual IViewportCameraController& Camera() noexcept = 0;
    [[nodiscard]] virtual IViewportManipulator& Manipulator() noexcept = 0;
    [[nodiscard]] virtual IViewportSnapProvider& Snap() noexcept = 0;
    [[nodiscard]] virtual IViewportGridProvider& Grid() noexcept = 0;

    [[nodiscard]] virtual undo::IUndoRuntime* Undo() noexcept = 0;
    [[nodiscard]] virtual undo::ITransactionManager* Transactions() noexcept = 0;
    [[nodiscard]] virtual property::IPropertyEditorRuntime* PropertyEditor() noexcept = 0;
    [[nodiscard]] virtual scene::Scene* Scene() noexcept = 0;
    [[nodiscard]] virtual world::IWorldRuntime* World() noexcept = 0;
    [[nodiscard]] virtual engine::EditorCamera* EditorCamera() noexcept = 0;

    [[nodiscard]] virtual ViewportToolId ActiveTool() const noexcept = 0;
    [[nodiscard]] virtual ViewportModeId ActiveMode() const noexcept = 0;
    [[nodiscard]] virtual std::string_view ActiveModeKey() const noexcept = 0;
    [[nodiscard]] virtual IViewportSelectionContext* SelectionContextPtr() noexcept { return nullptr; }
    [[nodiscard]] virtual float ViewportWidth() const noexcept = 0;
    [[nodiscard]] virtual float ViewportHeight() const noexcept = 0;

    virtual void SetViewportSize(float w, float h) = 0;
    virtual void NotifySelectionChanged() = 0;
};

class VIEWPORTEDIT_API IViewportInteraction {
public:
    virtual ~IViewportInteraction() = default;

    [[nodiscard]] virtual bool HandleMouseDown(const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleMouseUp(const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleMouseMove(const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleKeyDown(const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleKeyUp(const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool HandleScroll(const ViewportInputEvent& e) = 0;

    virtual void Tick(float deltaSeconds) = 0;
};

class VIEWPORTEDIT_API IViewportEditor {
public:
    virtual ~IViewportEditor() = default;

    [[nodiscard]] virtual IViewportContext& Context() noexcept = 0;
    [[nodiscard]] virtual IViewportInteraction& Interaction() noexcept = 0;
    [[nodiscard]] virtual IViewportSelection& Selection() noexcept = 0;
    [[nodiscard]] virtual IViewportHitTester& HitTester() noexcept = 0;
    [[nodiscard]] virtual IViewportCameraController& Camera() noexcept = 0;
    [[nodiscard]] virtual IViewportManipulator& Manipulator() noexcept = 0;
    [[nodiscard]] virtual IViewportSnapProvider& Snap() noexcept = 0;
    [[nodiscard]] virtual IViewportGridProvider& Grid() noexcept = 0;
    [[nodiscard]] virtual IViewportRenderer& Renderer() noexcept = 0;
    [[nodiscard]] virtual IViewportDragDrop& DragDrop() noexcept = 0;

    [[nodiscard]] virtual IViewportWorkspace& Workspace() noexcept = 0;
    [[nodiscard]] virtual IViewportModeManager& Modes() noexcept = 0;
    [[nodiscard]] virtual IViewportCommandRouter& Commands() noexcept = 0;
    [[nodiscard]] virtual IViewportSelectionContext& SelectionContext() noexcept = 0;
    [[nodiscard]] virtual IViewportToolContext& ToolContext() noexcept = 0;

    virtual void SetActiveTool(ViewportToolId tool) = 0;
    virtual void SetActiveTool(std::string_view toolId) = 0;
    [[nodiscard]] virtual ViewportToolId GetActiveTool() const noexcept = 0;
    virtual void SetActiveMode(ViewportModeId mode) = 0;
    virtual void SetActiveMode(std::string_view modeId) = 0;
    [[nodiscard]] virtual ViewportModeId GetActiveMode() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetActiveModeKey() const noexcept = 0;

    virtual void RegisterTool(std::unique_ptr<IViewportTool> tool) = 0;
    virtual void RegisterMode(std::unique_ptr<IViewportMode> mode) = 0;
    virtual void RegisterOverlay(std::unique_ptr<IViewportOverlay> overlay) = 0;

    virtual void Tick(float deltaSeconds) = 0;
    virtual void SetViewportSize(float w, float h) = 0;

    /// Bind Scene entity selection ↔ Outliner/Details (near-term bridge).
    virtual void SyncSelectionToScene() = 0;
    virtual void SyncSelectionFromScene() = 0;
};

} // namespace we::editor::viewportedit
