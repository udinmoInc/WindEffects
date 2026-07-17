#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/IServiceProvider.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Resources/IResourceRegistry.h"
#include "WindEffects/Runtime/UI/Events/IEventBus.h"
#include "WindEffects/Runtime/UI/Commands/ICommandRegistry.h"

namespace WindEffects::Editor::UI {

/// Application services shared by all WindEffects UI hosts (Launcher, Editor, tools).
/// Editor-only services (docking, extensions) live on IEditorApplicationContext.
class UI_API IApplicationContext {
public:
    virtual ~IApplicationContext() = default;

    [[nodiscard]] virtual IServiceProvider& GetServices() const = 0;
    [[nodiscard]] virtual IThemeProvider& GetThemeProvider() const = 0;
    [[nodiscard]] virtual IStyleResolver& GetStyleResolver() const = 0;
    [[nodiscard]] virtual IResourceRegistry& GetResourceRegistry() const = 0;
    [[nodiscard]] virtual IEventBus& GetEventBus() const = 0;
    [[nodiscard]] virtual ICommandRegistry& GetCommandRegistry() const = 0;
};

} // namespace WindEffects::Editor::UI
