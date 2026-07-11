#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Commands/ICommandRegistry.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace WindEffects::Editor::UI {
class Panel;
struct MenuItem;
class Widget;
}

namespace WindEffects::Editor::UI {

using ExtensionPanelFactory = std::function<std::shared_ptr<WindEffects::Editor::UI::Panel>()>;
using ExtensionMenuFactory = std::function<std::vector<std::shared_ptr<WindEffects::Editor::UI::MenuItem>>()>;

struct PanelRegistration {
    DockPanelDescriptor descriptor;
    ExtensionPanelFactory factory;
};

struct MenuRegistration {
    std::string menuName;
    ExtensionMenuFactory factory;
    int sortOrder = 0;
};

class UIFRAMEWORK_API IEditorUIExtension {
public:
    virtual ~IEditorUIExtension() = default;

    [[nodiscard]] virtual std::string_view GetExtensionId() const = 0;
    virtual void RegisterUI(class UIExtensionRegistry& registry) = 0;
};

class UIFRAMEWORK_API UIExtensionRegistry {
public:
    void RegisterExtension(std::shared_ptr<IEditorUIExtension> extension);
    void RegisterPanel(PanelRegistration registration);
    void RegisterMenu(MenuRegistration registration);
    void RegisterCommand(std::shared_ptr<ICommand> command);

    [[nodiscard]] const std::unordered_map<std::string, PanelRegistration>& GetPanels() const;
    [[nodiscard]] const std::vector<MenuRegistration>& GetMenus() const;

    void PopulateDockManager(IDockManager& dockManager) const;
    void PopulateCommandRegistry(ICommandRegistry& commands) const;

private:
    std::vector<std::shared_ptr<IEditorUIExtension>> m_Extensions;
    std::unordered_map<std::string, PanelRegistration> m_Panels;
    std::vector<MenuRegistration> m_Menus;
    std::vector<std::shared_ptr<ICommand>> m_PendingCommands;
};

} // namespace WindEffects::Editor::UI
