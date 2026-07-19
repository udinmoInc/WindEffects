#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/OutlinerTypes.h"
#include "WorldOutliner/IOutlinerNode.h"
#include "WorldOutliner/IOutlinerTreeModel.h"
#include "WorldOutliner/IOutlinerCommands.h"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::contentbrowser {
class TreeView;
}

namespace we::editor::outliner {

class WORLDOUTLINER_API IWorldOutliner {
public:
    virtual ~IWorldOutliner() = default;

    [[nodiscard]] virtual IOutlinerTreeModel& Model() noexcept = 0;
    [[nodiscard]] virtual IOutlinerSelection& Selection() noexcept = 0;
    [[nodiscard]] virtual IOutlinerSearch& Search() noexcept = 0;
    [[nodiscard]] virtual IOutlinerCommandRouter& Commands() noexcept = 0;
    [[nodiscard]] virtual IOutlinerEventRouter& Events() noexcept = 0;
    [[nodiscard]] virtual IOutlinerDragDropHandler& DragDrop() noexcept = 0;
    [[nodiscard]] virtual IOutlinerFolderProvider& Folders() noexcept = 0;

    virtual void RegisterDataProvider(std::unique_ptr<IOutlinerDataProvider> provider) = 0;
    virtual void RegisterColumn(std::unique_ptr<IOutlinerColumnProvider> column) = 0;
    virtual void RegisterDecorator(std::unique_ptr<IOutlinerDecorator> decorator) = 0;
    virtual void RegisterContextMenu(std::unique_ptr<IOutlinerContextMenuProvider> provider) = 0;
    virtual void RegisterFilter(std::unique_ptr<IOutlinerFilter> filter) = 0;

    virtual void BindTreeView(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& tree) = 0;
    virtual void RequestRebuild() = 0;
    virtual void Tick(float deltaSeconds) = 0;

    virtual void SetFilterState(const OutlinerFilterState& state) = 0;
    [[nodiscard]] virtual const OutlinerFilterState& GetFilterState() const noexcept = 0;

    virtual void SaveFilterPreset(std::string_view name) = 0;
    [[nodiscard]] virtual bool LoadFilterPreset(std::string_view name) = 0;
    [[nodiscard]] virtual std::vector<OutlinerFilterPreset> ListFilterPresets() const = 0;

    /// Sync bridges — called by runtime tick / event router; hosts must not wire ad-hoc selection.
    virtual void SyncSelectionToScene() = 0;
    virtual void SyncSelectionFromScene() = 0;
    virtual void SyncSelectionToViewport() = 0;
    virtual void SyncSelectionFromViewport() = 0;
};

class WORLDOUTLINER_API IWorldOutlinerRuntime {
public:
    virtual ~IWorldOutlinerRuntime() = default;

    [[nodiscard]] virtual IWorldOutliner& Outliner() noexcept = 0;
    [[nodiscard]] virtual const IWorldOutliner& Outliner() const noexcept = 0;
    virtual void Shutdown() = 0;
};

} // namespace we::editor::outliner
