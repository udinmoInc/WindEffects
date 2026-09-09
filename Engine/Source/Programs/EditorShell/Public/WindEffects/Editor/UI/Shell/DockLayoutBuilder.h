// ==============================================================================
// WindEffects — EditorShell — DockLayoutBuilder
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Panel/Panel.h"
#include "KindUI/Docking/DockContainer.h"

#include <unordered_map>

namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Splitter;
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::docking::DockContainer;
using ::we::editor::docking::WorkspaceLayout;
using ::we::editor::docking::DockLayoutNode;
using ::we::editor::extensions::UIExtensionRegistry;

struct DockLayoutBuildResult {
    std::shared_ptr<Widget> root;
    std::shared_ptr<Splitter> mainHorizontalSplitter;
    std::shared_ptr<Splitter> rootVerticalSplitter;
    std::shared_ptr<Splitter> leftCenterSplitter;
    std::shared_ptr<Splitter> toolsViewportSplitter;
    std::shared_ptr<Splitter> rightVerticalSplitter;
    std::shared_ptr<DockContainer> toolsDock;
    std::shared_ptr<DockContainer> viewportDock;
    std::shared_ptr<DockContainer> explorerDock;
    std::shared_ptr<DockContainer> detailsDock;
    std::shared_ptr<DockContainer> contentBrowserDock;
    std::unordered_map<std::string, std::shared_ptr<Panel>> panels;
};

class EDITORSHELL_API IDockLayoutBuilder {
public:
    virtual ~IDockLayoutBuilder() = default;
    virtual DockLayoutBuildResult Build(
        const WorkspaceLayout& layout,
        const UIExtensionRegistry& extensions,
        float dpiScale) = 0;
};

class EDITORSHELL_API DockLayoutBuilder final : public IDockLayoutBuilder {
public:
    DockLayoutBuildResult Build(
        const WorkspaceLayout& layout,
        const UIExtensionRegistry& extensions,
        float dpiScale) override;

private:
    std::shared_ptr<Widget> BuildNode(
        const DockLayoutNode& node,
        const UIExtensionRegistry& extensions,
        float dpiScale,
        DockLayoutBuildResult& result);

    std::shared_ptr<Panel> CreatePanel(
        std::string_view panelId,
        const UIExtensionRegistry& extensions,
        float dpiScale,
        DockLayoutBuildResult& result);
};

} // namespace we::editor::shell
