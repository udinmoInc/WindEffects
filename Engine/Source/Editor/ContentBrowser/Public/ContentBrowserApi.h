#pragma once

#include "ContentBrowser/Export.h"

namespace we::runtime::kindui { class IconRenderer; }

namespace we::programs::editor {

CONTENTBROWSER_API void InitializeContentBrowserService(::we::runtime::kindui::IconRenderer* iconRenderer);
CONTENTBROWSER_API void ShutdownContentBrowserService();

}
