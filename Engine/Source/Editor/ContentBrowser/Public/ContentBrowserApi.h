#pragma once

#include "ContentBrowser/Export.h"

namespace we::UI {
class IconRenderer;
}

namespace we::programs::editor {

CONTENTBROWSER_API void InitializeContentBrowserService(we::UI::IconRenderer* iconRenderer);
CONTENTBROWSER_API void ShutdownContentBrowserService();

}
