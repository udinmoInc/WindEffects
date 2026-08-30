#pragma once

#include "KindUI/Core/Widget.h"

#include <memory>

namespace we::programs::editor {

void RegisterEditorCompositionProbes(const std::shared_ptr<::we::runtime::kindui::Widget>& root);

} // namespace we::programs::editor
