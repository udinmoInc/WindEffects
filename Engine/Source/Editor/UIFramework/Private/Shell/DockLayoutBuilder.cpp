#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"

#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Widgets/DockContainer.h"
#include "KindUI/Layout/Splitter.h"
#include "WindEffects/Editor/UI/Core/PanelIconResolver.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Orientation;

using ::we::runtime::kindui::MetricToken;
namespace we::editor::shell {
using ::we::editor::docking::SplitOrientation;
using ::we::editor::docking::DockPanelDescriptor;
using ::we::editor::docking::DockLayoutNode;
using ::we::editor::docking::DockNodeType;
using ::we::editor::services::ResolvePanelTabIconName;

namespace {

Orientation ToOrientation(SplitOrientation orientation) {
    return orientation == SplitOrientation::Horizontal ? Orientation::Horizontal : Orientation::Vertical;
}

std::string ResolveTabIconName(const DockPanelDescriptor& descriptor) {
    return ResolvePanelTabIconName(descriptor.iconResource);
}

void ApplyPanelDescriptor(const std::shared_ptr<Panel>& panel, const DockPanelDescriptor& descriptor) {
    if (!panel) {
        return;
    }

    if (!descriptor.title.empty()) {
        panel->SetTitle(descriptor.title);
    }

    const std::string iconName = ResolveTabIconName(descriptor);
    if (!iconName.empty()) {
        panel->SetTabIcon(iconName);
    }
}

} // namespace

std::shared_ptr<Panel> DockLayoutBuilder::CreatePanel(
    std::string_view panelId,
    const UIExtensionRegistry& extensions,
    float dpiScale,
    DockLayoutBuildResult& result) {
    const auto& panels = extensions.GetPanels();
    const auto it = panels.find(std::string(panelId));
    if (it == panels.end()) {
        auto fallback = std::make_shared<Panel>(std::string(panelId));
        fallback->SetHeaderHeight(we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight) * dpiScale);
        result.panels[std::string(panelId)] = fallback;
        return fallback;
    }

    auto panel = it->second.factory();
    if (panel) {
        panel->SetHeaderHeight(we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight) * dpiScale);
        ApplyPanelDescriptor(panel, it->second.descriptor);
    }
    result.panels[std::string(panelId)] = panel;
    return panel;
}

std::shared_ptr<Widget> DockLayoutBuilder::BuildNode(
    const DockLayoutNode& node,
    const UIExtensionRegistry& extensions,
    float dpiScale,
    DockLayoutBuildResult& result) {
    switch (node.type) {
    case DockNodeType::Panel:
        return CreatePanel(node.panelId, extensions, dpiScale, result);
    case DockNodeType::TabGroup: {
        auto dock = std::make_shared<DockContainer>();
        if (auto panel = CreatePanel(node.panelId, extensions, dpiScale, result)) {
            dock->AddPanel(panel);
        }
        if (node.panelId == "Tools") {
            result.toolsDock = dock;
        } else if (node.panelId == "Viewport") {
            result.viewportDock = dock;
        } else if (node.panelId == "WorldOutliner") {
            result.explorerDock = dock;
        }
        return dock;
    }
    case DockNodeType::Split: {
        auto splitter = std::make_shared<Splitter>(ToOrientation(node.orientation), node.splitRatio);
        splitter->SetPanelGapEnabled(true);
        if (node.children.size() >= 1) {
            splitter->SetFirstChild(BuildNode(node.children[0], extensions, dpiScale, result));
        }
        if (node.children.size() >= 2) {
            splitter->SetSecondChild(BuildNode(node.children[1], extensions, dpiScale, result));
        }
        if (node.orientation == SplitOrientation::Horizontal && node.splitRatio > 0.7f) {
            result.mainHorizontalSplitter = splitter;
        } else if (node.orientation == SplitOrientation::Vertical && node.splitRatio > 0.6f) {
            result.leftCenterSplitter = splitter;
        } else if (node.orientation == SplitOrientation::Horizontal && node.splitRatio < 0.25f) {
            result.toolsViewportSplitter = splitter;
        } else if (node.orientation == SplitOrientation::Vertical && node.splitRatio < 0.5f) {
            result.rightVerticalSplitter = splitter;
        }
        return splitter;
    }
    case DockNodeType::Root:
    default:
        if (!node.children.empty()) {
            return BuildNode(node.children.front(), extensions, dpiScale, result);
        }
        return nullptr;
    }
}

DockLayoutBuildResult DockLayoutBuilder::Build(
    const WorkspaceLayout& layout,
    const UIExtensionRegistry& extensions,
    float dpiScale) {
    DockLayoutBuildResult result;
    result.root = BuildNode(layout.root, extensions, dpiScale, result);
    return result;
}

} // namespace we::editor::shell


