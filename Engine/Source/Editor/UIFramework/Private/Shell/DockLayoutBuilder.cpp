#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"

#include "Widgets/Panel.h"
#include "Widgets/DockContainer.h"
#include "Layout/Splitter.h"
#include "Core/Icon.h"

namespace WindEffects::Editor::UI {
namespace {

Orientation ToOrientation(SplitOrientation orientation) {
    return orientation == SplitOrientation::Horizontal ? Orientation::Horizontal : Orientation::Vertical;
}

std::string ResolveTabIconName(const DockPanelDescriptor& descriptor) {
    static const std::unordered_map<std::string, const char*> kAliases = {
        {"tools-panel", Icons::LayersName},
        {"viewport", Icons::PerspectiveName},
        {"outliner", Icons::HierarchyName},
        {"details", Icons::PropertiesName},
        {"content-browser", Icons::FolderName},
        {"output-log", Icons::ConsoleName},
    };

    if (descriptor.iconResource.empty()) {
        return {};
    }

    if (const auto it = kAliases.find(descriptor.iconResource); it != kAliases.end()) {
        return it->second;
    }

    return descriptor.iconResource;
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
        fallback->SetHeaderHeight(32.0f * dpiScale);
        result.panels[std::string(panelId)] = fallback;
        return fallback;
    }

    auto panel = it->second.factory();
    if (panel) {
        panel->SetHeaderHeight(32.0f * dpiScale);
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

} // namespace WindEffects::Editor::UI


