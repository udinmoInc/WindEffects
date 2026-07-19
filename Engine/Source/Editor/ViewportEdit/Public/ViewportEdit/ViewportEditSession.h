#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/IViewportEditor.h"

#include <memory>

namespace we::editor::viewportedit {

/// Editor-session binding for ViewportWidget / ToolsPanel that cannot take constructor DI.
class VIEWPORTEDIT_API ViewportEditSession {
public:
    static void Install(std::shared_ptr<IViewportEditor> editor);
    static void Clear() noexcept;

    [[nodiscard]] static IViewportEditor* Editor() noexcept;
    [[nodiscard]] static std::shared_ptr<IViewportEditor> EditorShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::editor::viewportedit
