#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/OutlinerTypes.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::outliner {

class WORLDOUTLINER_API IOutlinerNode {
public:
    virtual ~IOutlinerNode() = default;

    [[nodiscard]] virtual OutlinerNodeId GetId() const noexcept = 0;
    [[nodiscard]] virtual OutlinerNodeId GetParentId() const noexcept = 0;
    [[nodiscard]] virtual OutlinerNodeKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetTypeName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetIconName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetLayer() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::string> GetTags() const noexcept = 0;
    [[nodiscard]] virtual const OutlinerNodeFlags& GetFlags() const noexcept = 0;
    [[nodiscard]] virtual std::span<const OutlinerNodeId> GetChildren() const noexcept = 0;
};

class WORLDOUTLINER_API IOutlinerSelection {
public:
    virtual ~IOutlinerSelection() = default;

    [[nodiscard]] virtual std::span<const OutlinerNodeId> GetSelected() const noexcept = 0;
    [[nodiscard]] virtual OutlinerNodeId GetPrimary() const noexcept = 0;
    [[nodiscard]] virtual bool IsSelected(OutlinerNodeId id) const noexcept = 0;
    [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;
    [[nodiscard]] virtual std::size_t Count() const noexcept = 0;

    virtual void Clear() = 0;
    virtual void Set(OutlinerNodeId id) = 0;
    virtual void SetMany(std::span<const OutlinerNodeId> ids) = 0;
    virtual void Apply(OutlinerSelectionOp op, OutlinerNodeId id) = 0;

    [[nodiscard]] virtual std::span<const OutlinerNodeId> GetHistory() const noexcept = 0;
    virtual void PushHistory() = 0;
    virtual bool NavigateHistoryBack() = 0;
    virtual bool NavigateHistoryForward() = 0;
};

class WORLDOUTLINER_API IOutlinerFilter {
public:
    virtual ~IOutlinerFilter() = default;
    [[nodiscard]] virtual bool Passes(const IOutlinerNode& node, const OutlinerFilterState& state) const = 0;
};

class WORLDOUTLINER_API IOutlinerSearch {
public:
    virtual ~IOutlinerSearch() = default;
    virtual void SetQuery(std::string_view query) = 0;
    [[nodiscard]] virtual std::string_view GetQuery() const noexcept = 0;
    [[nodiscard]] virtual bool Matches(const IOutlinerNode& node) const = 0;
    [[nodiscard]] virtual std::vector<OutlinerNodeId> Query(std::span<const OutlinerNodeId> candidates) const = 0;
};

class WORLDOUTLINER_API IOutlinerSorter {
public:
    virtual ~IOutlinerSorter() = default;
    virtual void Sort(std::vector<OutlinerNodeId>& ids, OutlinerSortMode mode) const = 0;
};

} // namespace we::editor::outliner
