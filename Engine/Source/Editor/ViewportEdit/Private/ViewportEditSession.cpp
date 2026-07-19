#include "ViewportEdit/ViewportEditSession.h"

namespace we::editor::viewportedit {
namespace {

std::shared_ptr<IViewportEditor> g_Editor;

} // namespace

void ViewportEditSession::Install(std::shared_ptr<IViewportEditor> editor) {
    g_Editor = std::move(editor);
}

void ViewportEditSession::Clear() noexcept {
    g_Editor.reset();
}

IViewportEditor* ViewportEditSession::Editor() noexcept {
    return g_Editor.get();
}

std::shared_ptr<IViewportEditor> ViewportEditSession::EditorShared() noexcept {
    return g_Editor;
}

bool ViewportEditSession::IsInstalled() noexcept {
    return static_cast<bool>(g_Editor);
}

} // namespace we::editor::viewportedit
