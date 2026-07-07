#pragma once

#include "WorldOutliner/Export.h"

#include <volk.h>
#include <memory>

namespace we::UI {
class TreeView;
}

namespace we::programs::editor {

WORLDOUTLINER_API void BindExplorerBrandLogo(VkDescriptorSet logoSet, float logicalSize = 16.0f);
WORLDOUTLINER_API float GetExplorerDockTabLogoSize();
WORLDOUTLINER_API std::shared_ptr<we::UI::TreeView> GetExplorerTreeView();

} // namespace we::programs::editor
