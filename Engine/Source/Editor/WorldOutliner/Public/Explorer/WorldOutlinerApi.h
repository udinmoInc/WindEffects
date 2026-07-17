#pragma once

#include "WorldOutliner/Export.h"
#include "RHI/Types.h"

#include <memory>

namespace we::runtime::kindui {
class TreeView;
}

namespace we::programs::editor {

WORLDOUTLINER_API void BindExplorerBrandLogo(
    we::rhi::RHIDescriptorSetHandle logoSet,
    float logicalSize = 16.0f);
WORLDOUTLINER_API float GetExplorerDockTabLogoSize();
WORLDOUTLINER_API std::shared_ptr<we::runtime::kindui::TreeView> GetExplorerTreeView();

} // namespace we::programs::editor
