#include "Explorer/ExplorerPanelAssets.h"
#include "Explorer/WorldOutlinerApi.h"
#include "Widgets/TreeView.h"

#include <memory>

namespace we::programs::editor {

namespace {

VkDescriptorSet g_LogoSet = VK_NULL_HANDLE;
float g_LogoLogicalSize = 16.0f;
std::weak_ptr<we::UI::TreeView> g_ExplorerTreeView;

} // namespace

void BindExplorerBrandLogo(VkDescriptorSet logoSet, float logicalSize) {
    g_LogoSet = logoSet;
    g_LogoLogicalSize = logicalSize;
}

VkDescriptorSet GetExplorerBrandLogo() {
    return g_LogoSet;
}

float GetExplorerBrandLogoLogicalSize() {
    return g_LogoLogicalSize;
}

void RegisterExplorerTreeView(const std::shared_ptr<we::UI::TreeView>& treeView) {
    g_ExplorerTreeView = treeView;
}

std::shared_ptr<we::UI::TreeView> GetExplorerTreeView() {
    return g_ExplorerTreeView.lock();
}

float GetExplorerDockTabLogoSize() {
    return 17.0f;
}

} // namespace we::programs::editor
