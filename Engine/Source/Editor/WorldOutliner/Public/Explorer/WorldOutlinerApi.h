#pragma once

#include <volk.h>
#include <memory>

namespace we::UI {
class TreeView;
}

namespace we::programs::editor {

void BindExplorerBrandLogo(VkDescriptorSet logoSet, float logicalSize = 16.0f);
std::shared_ptr<we::UI::TreeView> GetExplorerTreeView();

} // namespace we::programs::editor
