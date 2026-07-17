#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/IApplicationContext.h"
#include "KindUI/Core/ServiceContainer.h"
#include "KindUI/Theming/IThemeProvider.h"
#include "KindUI/Resources/IResourceRegistry.h"
#include "KindUI/Events/IEventBus.h"
#include "KindUI/Commands/ICommandRegistry.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"

#include <memory>

namespace we::runtime::kindui {

class UIFRAMEWORK_API IEditorApplicationContext : public IApplicationContext {
public:
    ~IEditorApplicationContext() override = default;

    [[nodiscard]] virtual IDockManager& GetDockManager() const = 0;
    [[nodiscard]] virtual UIExtensionRegistry& GetExtensionRegistry() const = 0;
};

class EditorApplicationContext final
    : public ServiceContainer
    , public IEditorApplicationContext {
public:
    // Defaults to we::editor::ui::EditorTheme (orange accent).
    UIFRAMEWORK_API explicit EditorApplicationContext(
        std::shared_ptr<IThemeProvider> theme = nullptr);

    UIFRAMEWORK_API void Initialize(float dpiScale = 1.0f);
    UIFRAMEWORK_API void Shutdown();

    // Rebind context services after ThemeManager::SetTheme.
    UIFRAMEWORK_API void SyncThemeFromManager();

    [[nodiscard]] IServiceProvider& GetServices() const override { return *const_cast<EditorApplicationContext*>(this); }
    [[nodiscard]] IThemeProvider& GetThemeProvider() const override { return *m_ThemeProvider; }
    [[nodiscard]] IStyleResolver& GetStyleResolver() const override { return *m_StyleResolver; }
    [[nodiscard]] IResourceRegistry& GetResourceRegistry() const override { return *m_ResourceRegistry; }
    [[nodiscard]] IEventBus& GetEventBus() const override { return *m_EventBus; }
    [[nodiscard]] ICommandRegistry& GetCommandRegistry() const override { return *m_CommandRegistry; }
    [[nodiscard]] IDockManager& GetDockManager() const override { return *m_DockManager; }
    [[nodiscard]] UIExtensionRegistry& GetExtensionRegistry() const override { return *m_ExtensionRegistry; }

private:
    std::shared_ptr<IThemeProvider> m_ThemeProvider;
    std::shared_ptr<IStyleResolver> m_StyleResolver;
    std::shared_ptr<IResourceRegistry> m_ResourceRegistry;
    std::shared_ptr<IEventBus> m_EventBus;
    std::shared_ptr<ICommandRegistry> m_CommandRegistry;
    std::shared_ptr<IDockManager> m_DockManager;
    std::shared_ptr<UIExtensionRegistry> m_ExtensionRegistry;
};

} // namespace we::runtime::kindui
