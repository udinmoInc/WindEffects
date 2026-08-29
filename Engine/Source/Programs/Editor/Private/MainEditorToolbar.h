#pragma once

#include "EditorShellBuilder.h"

#include "KindUI/Core/Widget.h"

#include <memory>

namespace we::programs::editor {

[[nodiscard]] std::shared_ptr<::we::runtime::kindui::Widget> BuildMainEditorToolbar(
    const EditorShellDependencies& deps,
    const std::shared_ptr<::we::runtime::kindui::IWidgetContext>& widgetContext,
    float toolbarHeight,
    float leftInset,
    float rightInset,
    float edgePadding);

} // namespace we::programs::editor
