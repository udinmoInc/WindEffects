#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/IApplicationContext.h"
#include "KindUI/Core/ServiceContainer.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/ResolvedStyle.h"
#include "KindUI/Resources/IResourceRegistry.h"
#include "KindUI/Events/IEventBus.h"
#include "KindUI/Commands/ICommandRegistry.h"

#include <memory>

namespace we::runtime::kindui {

/// Lean application context for tools that do not need docking/extensions (Launcher, CrashReporter).
class KINDUI_API ApplicationContext
    : public ServiceContainer
    , public IApplicationContext {
public:
    explicit ApplicationContext(std::shared_ptr<IKindUITheme> theme = nullptr);

    void Initialize(float dpiScale = 1.0f);
    void Shutdown();
    void SyncThemeFromManager();

    [[nodiscard]] IServiceProvider& GetServices() const override {
        return *const_cast<ApplicationContext*>(this);
    }
    [[nodiscard]] IKindUITheme& GetTheme() const override { return *m_ThemeProvider; }
    [[nodiscard]] IStyleResolver& GetStyleResolver() const override { return *m_StyleResolver; }
    [[nodiscard]] IResourceRegistry& GetResourceRegistry() const override { return *m_ResourceRegistry; }
    [[nodiscard]] IEventBus& GetEventBus() const override { return *m_EventBus; }
    [[nodiscard]] ICommandRegistry& GetCommandRegistry() const override { return *m_CommandRegistry; }

protected:
    std::shared_ptr<IKindUITheme> m_ThemeProvider;
    std::shared_ptr<IStyleResolver> m_StyleResolver;
    std::shared_ptr<IResourceRegistry> m_ResourceRegistry;
    std::shared_ptr<IEventBus> m_EventBus;
    std::shared_ptr<ICommandRegistry> m_CommandRegistry;
};

} // namespace we::runtime::kindui
