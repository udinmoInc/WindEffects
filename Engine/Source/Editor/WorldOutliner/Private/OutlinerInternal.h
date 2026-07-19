#pragma once

#include "WorldOutliner/WorldOutliner.h"
#include "Undo/IUndoRuntime.h"
#include "Undo/ITransactionManager.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditor/IDetailsView.h"
#include "ViewportEdit/IViewportEditor.h"
#include "ViewportEdit/IViewportSelection.h"
#include "ViewportEdit/IViewportCameraController.h"
#include "ViewportEdit/ViewportEditSession.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "ContentBrowser/Widgets/TreeView.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::editor::outliner {
namespace detail {

[[nodiscard]] inline std::uint64_t NowMicros() noexcept {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch())
            .count());
}

[[nodiscard]] std::unique_ptr<IOutlinerDataProvider> CreateSceneDataProvider(scene::Scene* scene);
[[nodiscard]] std::unique_ptr<IOutlinerDataProvider> CreateWorldDataProvider(world::IWorldRuntime* world);

[[nodiscard]] std::unique_ptr<IOutlinerEventRouter> CreateEventRouter();
[[nodiscard]] std::unique_ptr<IOutlinerSelection> CreateSelection(IOutlinerEventRouter& events);
[[nodiscard]] std::unique_ptr<IOutlinerSearch> CreateSearch();
[[nodiscard]] std::unique_ptr<IOutlinerFilter> CreateDefaultFilter();
[[nodiscard]] std::unique_ptr<IOutlinerColumnProvider> CreateNameColumn();
[[nodiscard]] std::unique_ptr<IOutlinerColumnProvider> CreateTypeColumn();
[[nodiscard]] std::unique_ptr<IOutlinerColumnProvider> CreateLayerColumn();
[[nodiscard]] std::unique_ptr<IOutlinerColumnProvider> CreateTagColumn();
[[nodiscard]] std::unique_ptr<IOutlinerColumnProvider> CreateVisibilityColumn();
[[nodiscard]] std::unique_ptr<IOutlinerColumnProvider> CreateLockColumn();

[[nodiscard]] std::unique_ptr<IOutlinerTreeModel> CreateTreeModel();
[[nodiscard]] std::unique_ptr<IOutlinerFolderProvider> CreateFolderProvider(
    scene::Scene* scene, IOutlinerEventRouter& events, undo::ITransactionManager* tx);
[[nodiscard]] std::unique_ptr<IOutlinerCommandRouter> CreateCommandRouter(
    WorldOutlinerDependencies deps,
    IOutlinerSelection& selection,
    IOutlinerFolderProvider& folders,
    IOutlinerEventRouter& events,
    IOutlinerTreeModel& model);
[[nodiscard]] std::unique_ptr<IOutlinerDragDropHandler> CreateDragDrop(IOutlinerCommandRouter& commands);

void TreeModelSetProviders(IOutlinerTreeModel& model, std::vector<IOutlinerDataProvider*> providers);
void TreeModelSetFilters(IOutlinerTreeModel& model, std::vector<IOutlinerFilter*> filters);
void TreeModelSetSearch(IOutlinerTreeModel& model, IOutlinerSearch* search);

} // namespace detail
} // namespace we::editor::outliner
