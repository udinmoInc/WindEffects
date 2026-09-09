// ==============================================================================
// WindEffects — PropertyEditor — PropertyEditorSession
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "PropertyEditor/PropertyEditorSession.h"

namespace we::editor::property {
namespace {

std::shared_ptr<IPropertyEditorRuntime> g_Runtime;
std::shared_ptr<IDetailsView> g_Details;

} // namespace

void PropertyEditorSession::Install(
    std::shared_ptr<IPropertyEditorRuntime> runtime,
    std::shared_ptr<IDetailsView> detailsView)
{
    g_Runtime = std::move(runtime);
    g_Details = std::move(detailsView);
}

void PropertyEditorSession::Clear() noexcept {
    g_Details.reset();
    g_Runtime.reset();
}

IPropertyEditorRuntime* PropertyEditorSession::Runtime() noexcept {
    return g_Runtime.get();
}

IDetailsView* PropertyEditorSession::Details() noexcept {
    return g_Details.get();
}

std::shared_ptr<IDetailsView> PropertyEditorSession::DetailsShared() noexcept {
    return g_Details;
}

bool PropertyEditorSession::IsInstalled() noexcept {
    return g_Runtime != nullptr && g_Details != nullptr;
}

} // namespace we::editor::property
