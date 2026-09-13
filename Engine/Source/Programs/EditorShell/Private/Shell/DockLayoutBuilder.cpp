// ==============================================================================
// WindEffects — EditorShell — DockLayoutBuilder
// Internal implementation for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"

#include <KindUI/EditorUI.h>
#include "KindUI/Docking/DockContainer.h"
#include "WindEffects/Editor/UI/Core/PanelIconResolver.h"
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Orientation;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::DPIContext;

namespace we::editor::shell {
using ::we::editor::docking::SplitOrientation;
using ::we::editor::docking::DockPanelDescriptor;
using ::we::editor::docking::DockLayoutNode;
using ::we::editor::docking::DockNodeType;
using ::we::editor::services::ResolvePanelTabIcon;
using ::we::runtime::kindui::panels::PanelBuilder;

namespace {

Orientation ToOrientation(SplitOrientation orientation) {
    return orientation == SplitOrientation::Horizontal ? Orientation::Horizontal : Orientation::Vertical;
}

void ApplyPanelDescriptor(const std::shared_ptr<Panel>& panel, const DockPanelDescriptor& descriptor) {
    if (!panel) {
        return;
    }

    if (!descriptor.title.empty()) {
        panel->SetTitle(descriptor.title);
    }

    panel->SetTabIcon(ResolvePanelTabIcon(descriptor.id));
}

void WireSplitterSlot(const std::shared_ptr<we::runtime::kindui::Splitter>& splitter, const DockLayoutNode& node,
    DockLayoutBuildResult& result) {
    if (!splitter) {
        return;
    }

    const std::string& slot = node.slotId;
    if (slot == "mainHorizontal") {
        result.mainHorizontalSplitter = splitter;
    } else if (slot == "rootVertical") {
        result.rootVerticalSplitter = splitter;
    } else if (slot == "toolsViewport") {
        result.toolsViewportSplitter = splitter;
    } else if (slot == "rightVertical") {
        result.rightVerticalSplitter = splitter;
    }
}

} // namespace

std::shared_ptr<Panel> DockLayoutBuilder::CreatePanel(
    std::string_view panelId,
    const UIExtensionRegistry& extensions,
    float dpiScale,
    DockLayoutBuildResult& result) {
    (void)dpiScale;

    const auto& panels = extensions.GetPanels();
    const auto it = panels.find(std::string(panelId));
    if (it == panels.end()) {
        auto fallback = PanelBuilder::Create(std::string(panelId))
            .HeaderHeight(0.0f)
            .Build();
        result.panels[std::string(panelId)] = fallback;
        return fallback;
    }

    auto panel = it->second.factory();
    if (panel) {
        // Docked panels use the DockContainer tab strip — never a floating header.
        panel->SetHeaderHeight(0.0f);
        ApplyPanelDescriptor(panel, it->second.descriptor);
    }
    result.panels[std::string(panelId)] = panel;
    return panel;
}

std::shared_ptr<we::runtime::kindui::Widget> DockLayoutBuilder::BuildNode(
    const DockLayoutNode& node,
    const UIExtensionRegistry& extensions,
    float dpiScale,
    DockLayoutBuildResult& result) {
    switch (node.type) {
    case DockNodeType::Panel:
        return CreatePanel(node.panelId, extensions, dpiScale, result);
    case DockNodeType::TabGroup: {
        auto dock = std::make_shared<we::runtime::kindui::docking::DockContainer>();
        dock->SetHeaderHeightLogical(ResolveMetric(MetricToken::PanelTabHeight));
        if (auto panel = CreatePanel(node.panelId, extensions, dpiScale, result)) {
            dock->AddPanel(panel);
        }
        if (node.panelId == "Tools") {
            result.toolsDock = dock;
        } else if (node.panelId == "Viewport") {
            result.viewportDock = dock;
        } else if (node.panelId == "WorldOutliner") {
            result.explorerDock = dock;
        } else if (node.panelId == "Details") {
            result.detailsDock = dock;
        } else if (node.panelId == "ContentBrowser") {
            result.contentBrowserDock = dock;
        }
        return dock;
    }
    case DockNodeType::Split: {
        auto splitter = std::make_shared<we::runtime::kindui::Splitter>(ToOrientation(node.orientation),
            node.splitRatio);
        splitter->SetSlotId(node.slotId);
        splitter->SetPanelGapEnabled(true);
        splitter->SetMinPaneSizes(node.minFirstLogical * dpiScale, node.minSecondLogical * dpiScale);
        if (node.slotId == "rootVertical") {
            splitter->SetResizeMode(we::runtime::kindui::Splitter::ResizeMode::FixedSecond);
            splitter->SetFixedSecondWidth(240.0f * dpiScale);
            splitter->SetMinPaneSizes(200.0f * dpiScale, 140.0f * dpiScale);
        } else if (node.slotId == "toolsViewport") {
            splitter->SetResizeMode(we::runtime::kindui::Splitter::ResizeMode::FixedFirst);
            splitter->SetFixedFirstWidth(360.0f * dpiScale);
            splitter->SetMinPaneSizes(220.0f * dpiScale, 240.0f * dpiScale);
        } else if (node.slotId == "mainHorizontal") {
            splitter->SetResizeMode(we::runtime::kindui::Splitter::ResizeMode::FixedSecond);
            splitter->SetFixedSecondWidth(340.0f * dpiScale);
            splitter->SetMinPaneSizes(320.0f * dpiScale, 280.0f * dpiScale);
        }

        if (node.children.size() >= 1) {
            splitter->SetFirstChild(BuildNode(node.children[0], extensions, dpiScale, result));
        }
        if (node.children.size() >= 2) {
            splitter->SetSecondChild(BuildNode(node.children[1], extensions, dpiScale, result));
        }
        WireSplitterSlot(splitter, node, result);
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
    const float scale = dpiScale > 0.0f ? dpiScale : DPIContext::GetScale();
    result.root = BuildNode(layout.root, extensions, scale, result);
    return result;
}

} // namespace we::editor::shell
