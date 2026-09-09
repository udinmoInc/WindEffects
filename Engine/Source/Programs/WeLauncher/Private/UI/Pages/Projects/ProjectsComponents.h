// ==============================================================================
// WindEffects — WeLauncher — ProjectsComponents
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Declarative/Element.h"
#include "UI/Pages/Projects/ProjectsViewModel.h"

namespace we::programs::welauncher::projects {

/// Semantic page sections — each owns its own container; never mix title/toolbar/search.

[[nodiscard]] we::runtime::kindui::Element PageHeader();
[[nodiscard]] we::runtime::kindui::Element PageToolbar(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element PageDivider();
[[nodiscard]] we::runtime::kindui::Element ProjectTableHeader(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element ProjectTableBody(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element ProjectsEmptyState(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element PageContent(const ProjectsViewModel& vm);

} // namespace we::programs::welauncher::projects
