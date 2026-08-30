#pragma once

#include "Viewport/Export.h"
#include <memory>

namespace we::runtime::kindui {
class Widget;
}

namespace we::programs::editor {

/// Builds the viewport-local control strip (perspective, camera, transform, show).
VIEWPORT_API std::shared_ptr<::we::runtime::kindui::Widget> CreateViewportToolbar();

} // namespace we::programs::editor
