// ==============================================================================
// WindEffects — WeLauncher — LauncherSettingsStore
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Model/WeProjectDescriptor.h"

#include <vector>

namespace we::programs::welauncher {

class LauncherSettingsStore {
public:
    void Load();
    void Save();

    [[nodiscard]] LauncherSettings& Settings() { return m_Settings; }
    [[nodiscard]] const LauncherSettings& Settings() const { return m_Settings; }

    [[nodiscard]] const std::vector<std::string>& RecentProjects() const { return m_RecentProjects; }
    void TouchRecent(const std::string& weprojPath);
    void RemoveRecent(const std::string& weprojPath);
    void ResetToDefaults();

private:
    LauncherSettings m_Settings{};
    std::vector<std::string> m_RecentProjects;
};

} // namespace we::programs::welauncher
