#pragma once

#include <volk.h>
#include <memory>

namespace WindEffects::Editor::UI {
class TreeView;
}

namespace we::programs::editor {

VkDescriptorSet GetExplorerBrandLogo();
float GetExplorerBrandLogoLogicalSize();
void RegisterExplorerTreeView(const std::shared_ptr<WindEffects::Editor::UI::TreeView>& treeView);

} // namespace we::programs::editor
