#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

// Forward declarations for HouseUI elements
namespace we::UI {
    class Panel;
    class MenuItem;
    class Toolbar;
}

namespace we::programs::editor {

// A factory function that returns a constructed UI Panel
using PanelFactoryFunc = std::function<std::shared_ptr<we::UI::Panel>()>;
using MenuFactoryFunc = std::function<std::vector<std::shared_ptr<we::UI::MenuItem>>()>;

class EditorRegistry {
public:
    static EditorRegistry& Get();

    // Panels
    void RegisterPanel(std::string_view name, PanelFactoryFunc factory);
    [[nodiscard]] const std::unordered_map<std::string, PanelFactoryFunc>& GetPanels() const;

    // Menus
    void RegisterMenu(std::string_view menuName, MenuFactoryFunc factory);
    [[nodiscard]] const std::unordered_map<std::string, MenuFactoryFunc>& GetMenus() const;

private:
    EditorRegistry() = default;
    ~EditorRegistry() = default;

    std::unordered_map<std::string, PanelFactoryFunc> m_Panels;
    std::unordered_map<std::string, MenuFactoryFunc> m_Menus;
};

// Macro to easily register editor panels
#define REGISTER_EDITOR_PANEL(PanelName, FactoryFunc) \
    namespace { \
        struct EditorPanelRegister_##PanelName { \
            EditorPanelRegister_##PanelName() { \
                we::programs::editor::EditorRegistry::Get().RegisterPanel(#PanelName, FactoryFunc); \
            } \
        }; \
        static EditorPanelRegister_##PanelName g_EditorPanelRegister_##PanelName; \
    }

} // namespace we::programs::editor
