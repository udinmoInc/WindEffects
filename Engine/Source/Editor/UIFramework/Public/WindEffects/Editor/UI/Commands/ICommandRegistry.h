#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/IServiceProvider.h"

#include <functional>
#include <string>
#include <string_view>

namespace WindEffects::Editor::UI {

struct CommandContext {
    IServiceProvider* services = nullptr;
    std::string sourceId;
};

class UIFRAMEWORK_API ICommand {
public:
    virtual ~ICommand() = default;
    [[nodiscard]] virtual std::string_view GetId() const = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
    [[nodiscard]] virtual bool CanExecute(const CommandContext& context) const = 0;
    virtual void Execute(const CommandContext& context) = 0;
};

class UIFRAMEWORK_API ICommandRegistry {
public:
    virtual ~ICommandRegistry() = default;

    virtual void RegisterCommand(std::shared_ptr<ICommand> command) = 0;
    virtual void UnregisterCommand(std::string_view commandId) = 0;
    [[nodiscard]] virtual std::shared_ptr<ICommand> FindCommand(std::string_view commandId) const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<ICommand>> GetAllCommands() const = 0;
    virtual bool Execute(std::string_view commandId, const CommandContext& context) = 0;
};

} // namespace WindEffects::Editor::UI
