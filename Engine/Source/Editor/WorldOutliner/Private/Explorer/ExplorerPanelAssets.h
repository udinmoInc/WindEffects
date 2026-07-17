#pragma once

#include "RHI/Types.h"

#include <memory>

namespace we::runtime::kindui {
class TreeView;
}

namespace we::programs::editor {

we::rhi::RHIDescriptorSetHandle GetExplorerBrandLogo();
float GetExplorerBrandLogoLogicalSize();
void RegisterExplorerTreeView(const std::shared_ptr<we::runtime::kindui::TreeView>& treeView);

} // namespace we::programs::editor
