#pragma once

#include "KindUI/Declarative/Element.h"
#include "UI/Pages/Projects/ProjectsViewModel.h"

namespace we::programs::welauncher::projects {

[[nodiscard]] we::runtime::kindui::Element ActionToolbar(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element SearchToolbar(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element ProjectTableHeader(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element ProjectTableBody(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element ProjectsEmptyState(const ProjectsViewModel& vm);
[[nodiscard]] we::runtime::kindui::Element LoadingSkeleton(int rowCount);

} // namespace we::programs::welauncher::projects
