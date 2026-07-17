#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Commands/ICommandRegistry.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

class UI_API LambdaCommand final : public ICommand {
public:
    using ExecuteFn = std::function<void(const CommandContext&)>;
    using CanExecuteFn = std::function<bool(const CommandContext&)>;

    LambdaCommand(
        std::string id,
        std::string displayName,
        ExecuteFn execute,
        CanExecuteFn canExecute = {});

    [[nodiscard]] std::string_view GetId() const override { return m_Id; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return m_DisplayName; }
    [[nodiscard]] bool CanExecute(const CommandContext& context) const override;
    void Execute(const CommandContext& context) override;

private:
    std::string m_Id;
    std::string m_DisplayName;
    ExecuteFn m_Execute;
    CanExecuteFn m_CanExecute;
};

UI_API std::shared_ptr<ICommand> MakeCommand(
    std::string_view id,
    std::string_view displayName,
    LambdaCommand::ExecuteFn execute,
    LambdaCommand::CanExecuteFn canExecute = {});

} // namespace WindEffects::Editor::UI
