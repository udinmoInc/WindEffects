#include "EditorRegistry.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"

namespace we::programs::editor {

EditorRegistry& EditorRegistry::Get() {
    static EditorRegistry instance;
    return instance;
}

void EditorRegistry::RegisterPanel(std::string_view name, PanelFactoryFunc factory) {
    std::string strName{name};
    if (m_Panels.find(strName) == m_Panels.end()) {
        m_Panels.emplace(std::move(strName), std::move(factory));
        WE_LOG_TRACE(we::LogCategory::Editor.data(), std::string("[EditorRegistry] Registered Panel: ") + std::string(name));
    }
}

const std::unordered_map<std::string, PanelFactoryFunc>& EditorRegistry::GetPanels() const {
    return m_Panels;
}

void EditorRegistry::RegisterMenu(std::string_view menuName, MenuFactoryFunc factory) {
    std::string strName{menuName};
    if (m_Menus.find(strName) == m_Menus.end()) {
        m_Menus.emplace(std::move(strName), std::move(factory));
        WE_LOG_TRACE(we::LogCategory::Editor.data(), std::string("[EditorRegistry] Registered Menu: ") + std::string(menuName));
    }
}

const std::unordered_map<std::string, MenuFactoryFunc>& EditorRegistry::GetMenus() const {
    return m_Menus;
}

} // namespace we::programs::editor
