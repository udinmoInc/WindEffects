// ==============================================================================
// WindEffects — WeLauncher — ProjectsResources
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

namespace we::programs::welauncher::projects {

struct Resources {
    static constexpr const char* PageTitle = "Projects";
    static constexpr const char* PageSubtitle = "Open, create, and manage WindEffects projects";
    static constexpr const char* SearchPlaceholder = "Search Projects...";

    static constexpr const char* EmptyNoProjectsTitle = "No projects yet";
    static constexpr const char* EmptyNoProjectsSubtitle =
        "Create a new project or open an existing one.";

    static constexpr const char* EmptyNoMatchesTitle = "No matching projects";
    static constexpr const char* EmptyNoMatchesSubtitle =
        "Try a different search or clear the current query.";

    static constexpr const char* OpenProjectLabel = "Open Project";
    static constexpr const char* NewProjectLabel = "New Project";
    static constexpr const char* ClearSearchLabel = "Clear Search";
};

} // namespace we::programs::welauncher::projects
