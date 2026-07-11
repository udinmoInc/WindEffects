#pragma once

#include "ContentBrowser/Export.h"

namespace WindEffects::Editor::UI {
class IconRenderer;
}

namespace we::programs::editor {

CONTENTBROWSER_API void InitializeContentBrowserService(WindEffects::Editor::UI::IconRenderer* iconRenderer);
CONTENTBROWSER_API void ShutdownContentBrowserService();

}
