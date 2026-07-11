#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"

#include "EditorRegistry.h"
#include "Widgets/Panel.h"
#include "Widgets/DockContainer.h"
#include "Layout/Splitter.h"
#include "Layout/Box.h"

namespace WindEffects::Editor::UI {
namespace {

we::UI::Orientation ToLegacyOrientation(SplitOrientation orientation) {
    return orientation == SplitOrientation::Horizontal
        ? we::UI::Orientation::Horizontal
        : we::UI::Orientation::Vertical;
}

} // namespace

void BridgeLegacyEditorRegistry(UIExtensionRegistry& extensions) {
    const auto& legacyPanels = we::programs::editor::EditorRegistry::Get().GetPanels();
    for (const auto& [name, factory] : legacyPanels) {
        DockPanelDescriptor descriptor;
        descriptor.id = name;
        descriptor.title = name;
        descriptor.defaultVisible = true;

        if (name == "Tools") {
            descriptor.title = "Place Actors";
            descriptor.defaultZone = DockZone::Left;
        } else if (name == "Viewport") {
            descriptor.title = "Viewport 1";
            descriptor.defaultZone = DockZone::Center;
        } else if (name == "WorldOutliner") {
            descriptor.title = "Environment";
            descriptor.defaultZone = DockZone::Right;
        } else if (name == "Details") {
            descriptor.defaultZone = DockZone::Right;
        } else if (name == "ContentBrowser") {
            descriptor.defaultZone = DockZone::Bottom;
        } else {
            descriptor.defaultZone = DockZone::Floating;
        }

        extensions.RegisterPanel({descriptor, factory});
    }

    const auto& legacyMenus = we::programs::editor::EditorRegistry::Get().GetMenus();
    for (const auto& [menuName, factory] : legacyMenus) {
        extensions.RegisterMenu({std::string(menuName), factory});
    }
}

std::shared_ptr<we::UI::Panel> DockLayoutBuilder::CreatePanel(
    std::string_view panelId,
    const UIExtensionRegistry& extensions,
    float dpiScale,
    DockLayoutBuildResult& result) {
    const auto& panels = extensions.GetPanels();
    const auto it = panels.find(std::string(panelId));
    if (it == panels.end()) {
        auto fallback = std::make_shared<we::UI::Panel>(std::string(panelId));
        fallback->SetHeaderHeight(32.0f * dpiScale);
        result.panels[std::string(panelId)] = fallback;
        return fallback;
    }

    auto panel = it->second.factory();
    if (panel) {
        panel->SetHeaderHeight(32.0f * dpiScale);
    }
    result.panels[std::string(panelId)] = panel;
    return panel;
}

std::shared_ptr<we::UI::Widget> DockLayoutBuilder::BuildNode(
    const DockLayoutNode& node,
    const UIExtensionRegistry& extensions,
    float dpiScale,
    DockLayoutBuildResult& result) {
    switch (node.type) {
    case DockNodeType::Panel: {
        return CreatePanel(node.panelId, extensions, dpiScale, result);
    }
    case DockNodeType::TabGroup: {
        auto dock = std::make_shared<we::UI::DockContainer>();
        auto panel = CreatePanel(node.panelId, extensions, dpiScale, result);
        if (panel) {
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
        auto splitter = std::make_shared<we::UI::Splitter>(ToLegacyOrientation(node.orientation), node.splitRatio);
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
