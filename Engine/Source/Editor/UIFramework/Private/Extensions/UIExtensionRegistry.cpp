#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"

#include "WindEffects/Editor/UI/Commands/ICommandRegistry.h"

namespace WindEffects::Editor::UI {

void UIExtensionRegistry::RegisterExtension(std::shared_ptr<IEditorUIExtension> extension) {
    if (!extension) return;
    m_Extensions.push_back(extension);
    extension->RegisterUI(*this);
}

void UIExtensionRegistry::RegisterPanel(PanelRegistration registration) {
    m_Panels[registration.descriptor.id] = std::move(registration);
}

void UIExtensionRegistry::RegisterMenu(MenuRegistration registration) {
    m_Menus.push_back(std::move(registration));
}

void UIExtensionRegistry::RegisterCommand(std::shared_ptr<ICommand> command) {
    if (command) {
        m_PendingCommands.push_back(std::move(command));
    }
}

void UIExtensionRegistry::PopulateDockManager(IDockManager& dockManager) const {
    for (const auto& [id, reg] : m_Panels) {
        dockManager.RegisterPanel(reg.descriptor, [factory = reg.factory]() -> std::shared_ptr<void> {
            return factory();
        });
    }
}

void UIExtensionRegistry::PopulateCommandRegistry(ICommandRegistry& commands) const {
    for (const auto& command : m_PendingCommands) {
        commands.RegisterCommand(command);
    }
}

} // namespace WindEffects::Editor::UI
