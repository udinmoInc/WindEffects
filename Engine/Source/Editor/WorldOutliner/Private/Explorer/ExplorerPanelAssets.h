#pragma once

#include "RHI/Types.h"

#include <memory>

namespace WindEffects::Editor::UI {
class TreeView;
}

namespace we::programs::editor {

we::rhi::RHIDescriptorSetHandle GetExplorerBrandLogo();
float GetExplorerBrandLogoLogicalSize();
void RegisterExplorerTreeView(const std::shared_ptr<WindEffects::Editor::UI::TreeView>& treeView);

} // namespace we::programs::editor
