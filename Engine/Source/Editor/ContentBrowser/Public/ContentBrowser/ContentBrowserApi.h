// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserApi
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Export.h"

#include <string>

namespace we::runtime::kindui { class IconRenderer; }

namespace we::programs::editor {

/// Initialize content browser against an explicit project content root.
/// Never uses a hardcoded project path — caller must supply ProjectContext::ContentRoot().
CONTENTBROWSER_API void InitializeContentBrowserService(
    ::we::runtime::kindui::IconRenderer* iconRenderer,
    const std::string& contentRoot);

CONTENTBROWSER_API void ShutdownContentBrowserService();

}
