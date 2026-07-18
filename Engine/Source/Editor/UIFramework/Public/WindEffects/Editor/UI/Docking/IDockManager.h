#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::editor::docking {

enum class DockZone {
    Left,
    Center,
    Right,
    Bottom,
    Floating
};

struct DockPanelDescriptor {
    std::string id;
    std::string title;
    std::string iconResource;
    DockZone defaultZone = DockZone::Floating;
    bool closable = true;
    bool floatable = true;
    bool defaultVisible = true;
    int sortOrder = 0;
    bool showInWindowMenu = false;
    std::string windowMenuLabel;
};

using PanelFactory = std::function<std::shared_ptr<void>()>;

enum class DockNodeType {
    Split,
    TabGroup,
    Panel,
    Root
};

enum class SplitOrientation { Horizontal, Vertical };

struct DockLayoutNode {
    DockNodeType type = DockNodeType::Root;
    SplitOrientation orientation = SplitOrientation::Horizontal;
    /// Normalized first-pane share in [0,1]. Never store pixel positions here.
    float splitRatio = 0.5f;
    std::string panelId;
    std::vector<DockLayoutNode> children;
    /// Stable splitter identity for workspace wiring (not derived from ratios).
    /// Kept after `children` so the recursive prefix matches older layouts.
    std::string slotId;
    /// Logical minimum sizes (DPI-scaled at layout time) for first/second panes.
    float minFirstLogical = 120.0f;
    float minSecondLogical = 120.0f;
};

struct WorkspaceLayout {
    std::string workspaceId = "Default";
    DockLayoutNode root;
    std::unordered_map<std::string, DockPanelDescriptor> panels;
};

class UIFRAMEWORK_API IDockLayoutSerializer {
public:
    virtual ~IDockLayoutSerializer() = default;

    [[nodiscard]] virtual bool Save(const WorkspaceLayout& layout, std::string_view path) const = 0;
    [[nodiscard]] virtual bool Load(WorkspaceLayout& layout, std::string_view path) const = 0;
};

class UIFRAMEWORK_API IDockManager {
public:
    virtual ~IDockManager() = default;

    virtual void RegisterPanel(const DockPanelDescriptor& descriptor, PanelFactory factory) = 0;
    virtual void UnregisterPanel(std::string_view panelId) = 0;

    [[nodiscard]] virtual const WorkspaceLayout& GetLayout() const = 0;
    virtual void SetLayout(WorkspaceLayout layout) = 0;

    [[nodiscard]] virtual std::shared_ptr<void> GetPanelWidget(std::string_view panelId) const = 0;
    virtual void SetPanelVisible(std::string_view panelId, bool visible) = 0;
    [[nodiscard]] virtual bool IsPanelVisible(std::string_view panelId) const = 0;

    virtual void FloatPanel(std::string_view panelId) = 0;
    virtual void DockPanel(std::string_view panelId, DockZone zone) = 0;
};

} // namespace we::editor::docking
