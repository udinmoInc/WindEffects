#pragma once

#include "KindUI/Declarative/Element.h"
#include "UI/Pages/Projects/ProjectsViewModel.h"

namespace we::programs::welauncher {

[[nodiscard]] we::runtime::kindui::Element BuildProjectsView(const ProjectsViewModel& viewModel);

} // namespace we::programs::welauncher
