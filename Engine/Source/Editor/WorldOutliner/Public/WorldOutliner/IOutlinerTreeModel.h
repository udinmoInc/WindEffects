#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/OutlinerTypes.h"
#include "WorldOutliner/IOutlinerNode.h"

#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::outliner {

class WORLDOUTLINER_API IOutlinerDataProvider {
public:
    virtual ~IOutlinerDataProvider() = default;

    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual std::vector<OutlinerNodeId> GetRootIds() const = 0;
    [[nodiscard]] virtual const IOutlinerNode* FindNode(OutlinerNodeId id) const = 0;
    [[nodiscard]] virtual std::vector<OutlinerNodeId> GetChildren(OutlinerNodeId id) const = 0;
    virtual void EnsureChildrenLoaded(OutlinerNodeId id) = 0;
    virtual void Rebuild() = 0;
};

class WORLDOUTLINER_API IOutlinerTreeModel {
public:
    virtual ~IOutlinerTreeModel() = default;

    virtual void Rebuild() = 0;
    virtual void MarkDirty() = 0;
    [[nodiscard]] virtual bool IsDirty() const noexcept = 0;

    [[nodiscard]] virtual std::span<const OutlinerNodeId> GetVisibleRows() const noexcept = 0;
    [[nodiscard]] virtual const IOutlinerNode* GetNode(OutlinerNodeId id) const = 0;
    [[nodiscard]] virtual std::uint16_t GetDepth(OutlinerNodeId id) const noexcept = 0;

    virtual void SetExpanded(OutlinerNodeId id, bool expanded) = 0;
    [[nodiscard]] virtual bool IsExpanded(OutlinerNodeId id) const noexcept = 0;
    virtual void ExpandTo(OutlinerNodeId id) = 0;

    virtual void SetFilterState(const OutlinerFilterState& state) = 0;
    [[nodiscard]] virtual const OutlinerFilterState& GetFilterState() const noexcept = 0;
};

class WORLDOUTLINER_API IOutlinerFolderProvider {
public:
    virtual ~IOutlinerFolderProvider() = default;

    [[nodiscard]] virtual OutlinerNodeId CreateFolder(std::string_view name, OutlinerNodeId parent) = 0;
    [[nodiscard]] virtual bool RenameFolder(OutlinerNodeId id, std::string_view name) = 0;
    [[nodiscard]] virtual bool MoveFolder(OutlinerNodeId id, OutlinerNodeId newParent) = 0;
    [[nodiscard]] virtual bool DeleteFolder(OutlinerNodeId id) = 0;
};

class WORLDOUTLINER_API IOutlinerColumnProvider {
public:
    virtual ~IOutlinerColumnProvider() = default;

    [[nodiscard]] virtual OutlinerColumnId GetColumnId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    [[nodiscard]] virtual std::string GetCellText(const IOutlinerNode& node) const = 0;
    [[nodiscard]] virtual float GetPreferredWidth() const noexcept = 0;
};

class WORLDOUTLINER_API IOutlinerDecorator {
public:
    virtual ~IOutlinerDecorator() = default;
    [[nodiscard]] virtual std::string_view GetBadge(const IOutlinerNode& node) const = 0;
};

class WORLDOUTLINER_API IOutlinerContextMenuProvider {
public:
    virtual ~IOutlinerContextMenuProvider() = default;

    struct MenuItem {
        std::string id;
        std::string label;
        std::string shortcut;
        bool enabled = true;
    };

    [[nodiscard]] virtual std::vector<MenuItem> BuildMenu(std::span<const OutlinerNodeId> selection) const = 0;
    [[nodiscard]] virtual bool Execute(std::string_view itemId, std::span<const OutlinerNodeId> selection) = 0;
};

class WORLDOUTLINER_API IOutlinerDragDropHandler {
public:
    virtual ~IOutlinerDragDropHandler() = default;

    [[nodiscard]] virtual bool CanDrop(OutlinerNodeId dragged, OutlinerNodeId target) const = 0;
    [[nodiscard]] virtual bool Drop(OutlinerNodeId dragged, OutlinerNodeId target) = 0;
};

} // namespace we::editor::outliner
