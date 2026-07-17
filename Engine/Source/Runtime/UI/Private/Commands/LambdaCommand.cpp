#include "WindEffects/Runtime/UI/Commands/LambdaCommand.h"

namespace WindEffects::Editor::UI {

LambdaCommand::LambdaCommand(
    std::string id,
    std::string displayName,
    ExecuteFn execute,
    CanExecuteFn canExecute)
    : m_Id(std::move(id))
    , m_DisplayName(std::move(displayName))
    , m_Execute(std::move(execute))
    , m_CanExecute(std::move(canExecute)) {}

bool LambdaCommand::CanExecute(const CommandContext& context) const {
    return m_CanExecute ? m_CanExecute(context) : static_cast<bool>(m_Execute);
}

void LambdaCommand::Execute(const CommandContext& context) {
    if (m_Execute) {
        m_Execute(context);
    }
}

std::shared_ptr<ICommand> MakeCommand(
    std::string_view id,
    std::string_view displayName,
    LambdaCommand::ExecuteFn execute,
    LambdaCommand::CanExecuteFn canExecute) {
    return std::make_shared<LambdaCommand>(
        std::string(id),
        std::string(displayName),
        std::move(execute),
        std::move(canExecute));
}

} // namespace WindEffects::Editor::UI
