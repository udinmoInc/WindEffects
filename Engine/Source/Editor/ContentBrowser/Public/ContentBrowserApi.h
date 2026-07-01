#pragma once

namespace we::UI {
class IconRenderer;
}

namespace we::programs::editor {

void InitializeContentBrowserService(we::UI::IconRenderer* iconRenderer);
void ShutdownContentBrowserService();

} // namespace we::programs::editor
