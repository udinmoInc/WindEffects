#pragma once

#include <volk.h>
#include <memory>

namespace we::UI {
class TreeView;
}

namespace we::programs::editor {

VkDescriptorSet GetExplorerBrandLogo();
float GetExplorerBrandLogoLogicalSize();
void RegisterExplorerTreeView(const std::shared_ptr<we::UI::TreeView>& treeView);

} // namespace we::programs::editor
