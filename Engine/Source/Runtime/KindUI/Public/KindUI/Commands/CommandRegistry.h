#pragma once

#include "KindUI/Export.h"
#include "KindUI/Commands/ICommandRegistry.h"

#include <mutex>
#include <unordered_map>

namespace we::runtime::kindui {

class KINDUI_API CommandRegistry final : public ICommandRegistry {
public:
    void RegisterCommand(std::shared_ptr<ICommand> command) override;
    void UnregisterCommand(std::string_view commandId) override;
    [[nodiscard]] std::shared_ptr<ICommand> FindCommand(std::string_view commandId) const override;
    [[nodiscard]] std::vector<std::shared_ptr<ICommand>> GetAllCommands() const override;
    bool Execute(std::string_view commandId, const CommandContext& context) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<ICommand>> m_Commands;
};

} // namespace we::runtime::kindui
