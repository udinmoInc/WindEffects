#pragma once

#include "ViewportEdit/ViewportEdit.h"
#include "Undo/IUndoRuntime.h"
#include "Undo/ITransactionManager.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "Scene/Scene.h"
#include "EditorCamera.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::editor::viewportedit {
namespace detail {

[[nodiscard]] inline std::uint64_t NowMicros() noexcept {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch())
            .count());
}

[[nodiscard]] std::unique_ptr<IViewportSelection> CreateSelection();
[[nodiscard]] std::unique_ptr<IViewportHitTester> CreateHitTester(IViewportContext& context);
[[nodiscard]] std::unique_ptr<IViewportCameraController> CreateCameraController(IViewportContext& context);
[[nodiscard]] std::unique_ptr<IViewportSnapProvider> CreateSnapProvider(SnapSettings settings);
[[nodiscard]] std::unique_ptr<IViewportGridProvider> CreateGridProvider(float cellSize);
[[nodiscard]] std::unique_ptr<IViewportManipulator> CreateManipulator(IViewportContext& context);
[[nodiscard]] std::unique_ptr<IViewportRenderer> CreateRenderer();
[[nodiscard]] std::unique_ptr<IViewportDragDrop> CreateDragDrop(IViewportContext& context);
[[nodiscard]] std::unique_ptr<IViewportInteraction> CreateInteraction(IViewportContext& context);
[[nodiscard]] std::unique_ptr<IViewportTool> CreateSelectTool();
[[nodiscard]] std::unique_ptr<IViewportTool> CreateMoveTool();
[[nodiscard]] std::unique_ptr<IViewportTool> CreateRotateTool();
[[nodiscard]] std::unique_ptr<IViewportTool> CreateScaleTool();
[[nodiscard]] std::unique_ptr<IViewportMode> CreateDefaultMode();

void BindInteractionTool(IViewportInteraction& interaction, IViewportTool* tool);

} // namespace detail
} // namespace we::editor::viewportedit
