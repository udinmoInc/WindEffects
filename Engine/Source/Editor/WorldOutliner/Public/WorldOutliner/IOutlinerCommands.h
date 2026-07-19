#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/OutlinerTypes.h"

#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::outliner {

struct WORLDOUTLINER_API OutlinerEvent {
    OutlinerEventKind kind = OutlinerEventKind::SelectionChanged;
    std::vector<OutlinerNodeId> nodes;
    OutlinerNodeId primary{};
    std::string detail;
};

using OutlinerEventListener = std::function<void(const OutlinerEvent&)>;

/// Central event bus — selection/hierarchy changes publish here; ViewportEdit / PE / Details subscribe.
class WORLDOUTLINER_API IOutlinerEventRouter {
public:
    virtual ~IOutlinerEventRouter() = default;

    virtual void Publish(const OutlinerEvent& event) = 0;
    virtual void Subscribe(OutlinerEventListener listener) = 0;
    virtual void Suspend(bool suspend) = 0;
    [[nodiscard]] virtual bool IsSuspended() const noexcept = 0;
};

class WORLDOUTLINER_API IOutlinerCommandRouter {
public:
    virtual ~IOutlinerCommandRouter() = default;

    [[nodiscard]] virtual bool Rename(OutlinerNodeId id, std::string_view newName) = 0;
    [[nodiscard]] virtual bool DeleteSelected() = 0;
    [[nodiscard]] virtual bool DuplicateSelected() = 0;
    [[nodiscard]] virtual bool Reparent(OutlinerNodeId child, OutlinerNodeId newParent) = 0;
    [[nodiscard]] virtual bool CopySelected() = 0;
    [[nodiscard]] virtual bool Paste(OutlinerNodeId parent) = 0;
    [[nodiscard]] virtual bool FocusSelected() = 0;
    [[nodiscard]] virtual bool RevealInViewport() = 0;
    [[nodiscard]] virtual bool RevealInContentBrowser() = 0;
    [[nodiscard]] virtual bool CreateFolder(std::string_view name) = 0;
    [[nodiscard]] virtual bool SetVisible(OutlinerNodeId id, bool visible) = 0;
    [[nodiscard]] virtual bool SetLocked(OutlinerNodeId id, bool locked) = 0;
    [[nodiscard]] virtual bool ToggleFavorite(OutlinerNodeId id) = 0;
    [[nodiscard]] virtual bool TogglePinned(OutlinerNodeId id) = 0;
};

} // namespace we::editor::outliner
