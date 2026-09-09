// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserSession
// Internal implementation for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "ContentBrowser/ContentBrowserSession.h"

namespace we::editor::contentbrowser {
namespace {

std::shared_ptr<IContentBrowserRuntime> g_Runtime;

} // namespace

void ContentBrowserSession::Install(std::shared_ptr<IContentBrowserRuntime> runtime) {
    g_Runtime = std::move(runtime);
}

void ContentBrowserSession::Clear() noexcept {
    g_Runtime.reset();
}

IContentBrowserRuntime* ContentBrowserSession::Runtime() noexcept {
    return g_Runtime.get();
}

IContentBrowser* ContentBrowserSession::Browser() noexcept {
    return g_Runtime ? &g_Runtime->Browser() : nullptr;
}

std::shared_ptr<IContentBrowserRuntime> ContentBrowserSession::RuntimeShared() noexcept {
    return g_Runtime;
}

bool ContentBrowserSession::IsInstalled() noexcept {
    return static_cast<bool>(g_Runtime);
}

} // namespace we::editor::contentbrowser
