// ==============================================================================
// WindEffects — EditorShell — ExtensionBootstrap
// Internal implementation for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"

namespace we::editor::extensions {

ExtensionBootstrap& ExtensionBootstrap::Instance() {
    static ExtensionBootstrap bootstrap;
    return bootstrap;
}

void ExtensionBootstrap::RegisterPanel(PanelRegistration registration) {
    m_PendingPanels.push_back(std::move(registration));
}

void ExtensionBootstrap::RegisterMenu(MenuRegistration registration) {
    m_PendingMenus.push_back(std::move(registration));
}

void ExtensionBootstrap::RegisterCommand(std::shared_ptr<ICommand> command) {
    if (command) {
        m_PendingCommands.push_back(std::move(command));
    }
}

void ExtensionBootstrap::FlushTo(UIExtensionRegistry& registry) {
    for (auto& panel : m_PendingPanels) {
        registry.RegisterPanel(std::move(panel));
    }
    m_PendingPanels.clear();

    for (auto& menu : m_PendingMenus) {
        registry.RegisterMenu(std::move(menu));
    }
    m_PendingMenus.clear();

    for (auto& command : m_PendingCommands) {
        registry.RegisterCommand(std::move(command));
    }
    m_PendingCommands.clear();
}

} // namespace we::editor::extensions


