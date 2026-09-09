// ==============================================================================
// WindEffects — WeLauncher — ProjectsView
// UI widget used by the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Declarative/Element.h"
#include "UI/Pages/Projects/ProjectsViewModel.h"

namespace we::programs::welauncher {

[[nodiscard]] we::runtime::kindui::Element BuildProjectsView(const ProjectsViewModel& viewModel);

} // namespace we::programs::welauncher
