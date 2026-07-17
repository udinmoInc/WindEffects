#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/IApplicationContext.h"
#include "WindEffects/Runtime/UI/Core/ServiceContainer.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Resources/IResourceRegistry.h"
#include "WindEffects/Runtime/UI/Events/IEventBus.h"
#include "WindEffects/Runtime/UI/Commands/ICommandRegistry.h"

#include <memory>

namespace WindEffects::Editor::UI {

/// Lean application context for tools that do not need docking/extensions (Launcher, CrashReporter).
class UI_API ApplicationContext
    : public ServiceContainer
    , public IApplicationContext {
public:
    explicit ApplicationContext(std::shared_ptr<IThemeProvider> theme = nullptr);

    void Initialize(float dpiScale = 1.0f);
    void Shutdown();
    void SyncThemeFromManager();

    [[nodiscard]] IServiceProvider& GetServices() const override {
        return *const_cast<ApplicationContext*>(this);
    }
    [[nodiscard]] IThemeProvider& GetThemeProvider() const override { return *m_ThemeProvider; }
    [[nodiscard]] IStyleResolver& GetStyleResolver() const override { return *m_StyleResolver; }
    [[nodiscard]] IResourceRegistry& GetResourceRegistry() const override { return *m_ResourceRegistry; }
    [[nodiscard]] IEventBus& GetEventBus() const override { return *m_EventBus; }
    [[nodiscard]] ICommandRegistry& GetCommandRegistry() const override { return *m_CommandRegistry; }

protected:
    std::shared_ptr<IThemeProvider> m_ThemeProvider;
    std::shared_ptr<IStyleResolver> m_StyleResolver;
    std::shared_ptr<IResourceRegistry> m_ResourceRegistry;
    std::shared_ptr<IEventBus> m_EventBus;
    std::shared_ptr<ICommandRegistry> m_CommandRegistry;
};

} // namespace WindEffects::Editor::UI
