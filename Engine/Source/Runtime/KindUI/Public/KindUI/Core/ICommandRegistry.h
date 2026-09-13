// ==============================================================================
// WindEffects — KindUI — ICommandRegistry
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/IServiceProvider.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::kindui {

struct CommandContext {
    IServiceProvider* services = nullptr;
    std::string sourceId;
};

class KINDUI_API ICommand {
public:
    virtual ~ICommand() = default;
    [[nodiscard]] virtual std::string_view GetId() const = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
    [[nodiscard]] virtual bool CanExecute(const CommandContext& context) const = 0;
    virtual void Execute(const CommandContext& context) = 0;
};

class KINDUI_API ICommandRegistry {
public:
    virtual ~ICommandRegistry() = default;

    virtual void RegisterCommand(std::shared_ptr<ICommand> command) = 0;
    virtual void UnregisterCommand(std::string_view commandId) = 0;
    [[nodiscard]] virtual std::shared_ptr<ICommand> FindCommand(std::string_view commandId) const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<ICommand>> GetAllCommands() const = 0;
    virtual bool Execute(std::string_view commandId, const CommandContext& context) = 0;
};

} // namespace we::runtime::kindui
