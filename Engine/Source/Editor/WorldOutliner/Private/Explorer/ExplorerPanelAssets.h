#pragma once

#include "RHI/Types.h"

#include <memory>

namespace we::editor::outliner {
class TreeView;
}

namespace we::programs::editor {

we::rhi::RHIDescriptorSetHandle GetExplorerBrandLogo();
float GetExplorerBrandLogoLogicalSize();
void RegisterExplorerTreeView(const std::shared_ptr<::we::editor::contentbrowser::TreeView>& treeView);

} // namespace we::programs::editor
