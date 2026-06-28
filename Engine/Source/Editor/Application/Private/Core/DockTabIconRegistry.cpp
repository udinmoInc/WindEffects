#include "Core/DockTabIconRegistry.hpp"

namespace we::UI {

DockTabIconRegistry& DockTabIconRegistry::Get() {
    static DockTabIconRegistry instance;
    return instance;
}

void DockTabIconRegistry::RegisterIcon(const std::string& panelTitle, const std::string& iconName) {
    m_PanelIcons[panelTitle] = iconName;
}

std::string DockTabIconRegistry::GetIcon(const std::string& panelTitle) const {
    auto it = m_PanelIcons.find(panelTitle);
    if (it != m_PanelIcons.end()) {
        return it->second;
    }
    return "";
}

bool DockTabIconRegistry::HasIcon(const std::string& panelTitle) const {
    return m_PanelIcons.find(panelTitle) != m_PanelIcons.end();
}

} // namespace we::editor::application::UI
