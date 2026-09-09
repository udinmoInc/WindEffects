// ==============================================================================
// WindEffects — KindUI — IApplicationContext
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/IServiceProvider.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/ResolvedStyle.h"
#include "KindUI/Resources/IResourceRegistry.h"
#include "KindUI/Events/IEventBus.h"
#include "KindUI/Commands/ICommandRegistry.h"

namespace we::runtime::kindui {

/// Application services shared by all WindEffects UI hosts (Launcher, Editor, tools).
/// Editor-only services (docking, extensions) live on IEditorApplicationContext.
class KINDUI_API IApplicationContext {
public:
    virtual ~IApplicationContext() = default;

    [[nodiscard]] virtual IServiceProvider& GetServices() const = 0;
    [[nodiscard]] virtual IKindUITheme& GetTheme() const = 0;
    [[nodiscard]] virtual IStyleResolver& GetStyleResolver() const = 0;
    [[nodiscard]] virtual IResourceRegistry& GetResourceRegistry() const = 0;
    [[nodiscard]] virtual IEventBus& GetEventBus() const = 0;
    [[nodiscard]] virtual ICommandRegistry& GetCommandRegistry() const = 0;
};

} // namespace we::runtime::kindui
