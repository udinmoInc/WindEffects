// ==============================================================================
// WindEffects — WorldOutliner — ExplorerPanelAssets
// Internal implementation for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Explorer/ExplorerPanelAssets.h"
#include "Explorer/WorldOutlinerApi.h"
#include "ContentBrowser/Widgets/TreeView.h"

#include <memory>

namespace we::programs::editor {

namespace {

we::rhi::RHIDescriptorSetHandle g_LogoSet = we::rhi::RHIDescriptorSetHandle::Invalid;
float g_LogoLogicalSize = 16.0f;
std::weak_ptr<::we::editor::contentbrowser::TreeView> g_ExplorerTreeView;

} // namespace

void BindExplorerBrandLogo(we::rhi::RHIDescriptorSetHandle logoSet, float logicalSize) {
    g_LogoSet = logoSet;
    g_LogoLogicalSize = logicalSize;
}

we::rhi::RHIDescriptorSetHandle GetExplorerBrandLogo() {
    return g_LogoSet;
}

float GetExplorerBrandLogoLogicalSize() {
    return g_LogoLogicalSize;
}

void RegisterExplorerTreeView(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& treeView) {
    g_ExplorerTreeView = treeView;
}

std::shared_ptr<::we::editor::contentbrowser::TreeView> GetExplorerTreeView() {
    return g_ExplorerTreeView.lock();
}

float GetExplorerDockTabLogoSize() {
    return 17.0f;
}

} // namespace we::programs::editor
