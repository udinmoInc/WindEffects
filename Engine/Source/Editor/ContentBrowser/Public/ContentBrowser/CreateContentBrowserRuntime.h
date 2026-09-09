// ==============================================================================
// WindEffects — ContentBrowser — CreateContentBrowserRuntime
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/ContentBrowserTypes.h"
#include "ContentBrowser/IContentBrowser.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace we::runtime::kindui {
class IconRenderer;
}

namespace we::runtime::assetruntime {
class IAssetManager;
}

namespace we::editor::contentbrowser {

/// Host-injected reversible transaction (Editor wires Undo::RecordCustom).
using ContentBrowserTransactionFn = std::function<bool(
    std::string_view label,
    std::function<bool()> undoFn,
    std::function<bool()> redoFn)>;

struct CONTENTBROWSER_API ContentBrowserDependencies {
    we::runtime::kindui::IconRenderer* iconRenderer = nullptr;
    std::filesystem::path contentRoot;
    assetruntime::IAssetManager* assetManager = nullptr;
    ContentBrowserTransactionFn recordTransaction;
    ContentBrowserConfig config{};
    std::function<void(std::string_view)> onLog;
};

[[nodiscard]] CONTENTBROWSER_API std::unique_ptr<IContentBrowserRuntime> CreateContentBrowserRuntime(
    const ContentBrowserDependencies& deps);

} // namespace we::editor::contentbrowser
