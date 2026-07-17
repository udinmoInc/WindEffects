#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Commands/ICommandRegistry.h"
#include "KindUI/Core/Widget.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::editor::menus {
struct MenuItem;
}

namespace we::editor::extensions {

using ::we::runtime::kindui::ICommand;
using ::we::runtime::kindui::ICommandRegistry;

using ExtensionPanelFactory = std::function<std::shared_ptr<::we::editor::panels::Panel>()>;
using ExtensionMenuFactory = std::function<std::vector<std::shared_ptr<::we::editor::menus::MenuItem>>()>;

struct PanelRegistration {
    ::we::editor::docking::DockPanelDescriptor descriptor;
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

    void PopulateDockManager(::we::editor::docking::IDockManager& dockManager) const;
    void PopulateCommandRegistry(ICommandRegistry& commands) const;

private:
    std::vector<std::shared_ptr<IEditorUIExtension>> m_Extensions;
    std::unordered_map<std::string, PanelRegistration> m_Panels;
    std::vector<MenuRegistration> m_Menus;
    std::vector<std::shared_ptr<ICommand>> m_PendingCommands;
};

} // namespace we::editor::extensions
