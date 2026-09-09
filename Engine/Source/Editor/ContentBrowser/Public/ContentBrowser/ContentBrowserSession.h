// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserSession
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/IContentBrowser.h"

#include <memory>

namespace we::editor::contentbrowser {

/// Editor-session binding for panel factories that cannot take constructor DI.
class CONTENTBROWSER_API ContentBrowserSession {
public:
    static void Install(std::shared_ptr<IContentBrowserRuntime> runtime);
    static void Clear() noexcept;

    [[nodiscard]] static IContentBrowserRuntime* Runtime() noexcept;
    [[nodiscard]] static IContentBrowser* Browser() noexcept;
    [[nodiscard]] static std::shared_ptr<IContentBrowserRuntime> RuntimeShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::editor::contentbrowser
