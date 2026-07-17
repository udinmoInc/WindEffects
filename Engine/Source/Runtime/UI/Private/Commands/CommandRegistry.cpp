#include "WindEffects/Runtime/UI/Commands/CommandRegistry.h"

namespace WindEffects::Editor::UI {

void CommandRegistry::RegisterCommand(std::shared_ptr<ICommand> command) {
    if (!command) return;
    std::lock_guard lock(m_Mutex);
    m_Commands[std::string(command->GetId())] = std::move(command);
}

void CommandRegistry::UnregisterCommand(std::string_view commandId) {
    std::lock_guard lock(m_Mutex);
    m_Commands.erase(std::string(commandId));
}

std::shared_ptr<ICommand> CommandRegistry::FindCommand(std::string_view commandId) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Commands.find(std::string(commandId));
    return it != m_Commands.end() ? it->second : nullptr;
}

std::vector<std::shared_ptr<ICommand>> CommandRegistry::GetAllCommands() const {
    std::lock_guard lock(m_Mutex);
    std::vector<std::shared_ptr<ICommand>> result;
    result.reserve(m_Commands.size());
    for (const auto& [_, cmd] : m_Commands) {
        result.push_back(cmd);
    }
    return result;
}

bool CommandRegistry::Execute(std::string_view commandId, const CommandContext& context) {
    auto command = FindCommand(commandId);
    if (!command || !command->CanExecute(context)) {
        return false;
    }
    command->Execute(context);
    return true;
}

} // namespace WindEffects::Editor::UI

// export rebuild
