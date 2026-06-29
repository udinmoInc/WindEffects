#include "EditorRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Core/Icon.hpp"
#include "Widgets/Label.hpp"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateViewportPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Viewport");
    panel->SetHeaderHeight(30.0f);
    panel->AddHeaderAction(we::UI::Icons::XName, []() {});

    // ViewportWidget requires Renderer, Camera, Scene, UIRenderer which
    // are not available at static registration time. Set an empty placeholder
    // content for now — the real ViewportWidget will be injected later by
    // the editor application once the runtime services are initialized.
    auto placeholder = std::make_shared<we::UI::Label>("");
    panel->SetContent(placeholder);

    return panel;
}

REGISTER_EDITOR_PANEL(Viewport, CreateViewportPanel)

} // namespace we::programs::editor
