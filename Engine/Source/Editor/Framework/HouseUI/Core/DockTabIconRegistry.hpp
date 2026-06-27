#pragma once

#include <string>
#include <unordered_map>

namespace HouseEngine::UI {

// Centralized registry for dock tab icons
class DockTabIconRegistry {
public:
    static DockTabIconRegistry& Get();

    // Register an icon for a specific panel title
    void RegisterIcon(const std::string& panelTitle, const std::string& iconName);

    // Get the icon name for a specific panel title. Returns empty string if not found.
    std::string GetIcon(const std::string& panelTitle) const;

    // Check if a panel has a registered icon
    bool HasIcon(const std::string& panelTitle) const;

private:
    DockTabIconRegistry() = default;
    
    std::unordered_map<std::string, std::string> m_PanelIcons;
};

} // namespace HouseEngine::UI
