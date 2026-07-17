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
