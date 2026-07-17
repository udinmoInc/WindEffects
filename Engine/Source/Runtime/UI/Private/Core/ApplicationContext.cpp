#include "WindEffects/Runtime/UI/Core/ApplicationContext.h"

#include "WindEffects/Runtime/UI/Theming/GraphiteDarkTheme.h"
#include "WindEffects/Runtime/UI/Theming/ThemeManager.h"
#include "WindEffects/Runtime/UI/Resources/ModuleResourceRegistry.h"
#include "WindEffects/Runtime/UI/Events/EventBus.h"
#include "WindEffects/Runtime/UI/Commands/CommandRegistry.h"
#include "WindEffects/Runtime/UI/Core/DPIContext.h"

namespace WindEffects::Editor::UI {

ApplicationContext::ApplicationContext(std::shared_ptr<IThemeProvider> theme) {
    if (!theme) {
        theme = std::make_shared<GraphiteDarkTheme>();
    }
    ThemeManager::Get().Initialize(std::move(theme), 1.0f);
    m_ThemeProvider = ThemeManager::Get().SharedProvider();
    m_StyleResolver = ThemeManager::Get().SharedResolver();
    m_ResourceRegistry = std::make_shared<ModuleResourceRegistry>();
    m_EventBus = std::make_shared<EventBus>();
    m_CommandRegistry = std::make_shared<CommandRegistry>();

    RegisterService(typeid(IThemeProvider), m_ThemeProvider);
    RegisterService(typeid(IStyleResolver), m_StyleResolver);
    RegisterService(typeid(IResourceRegistry), m_ResourceRegistry);
    RegisterService(typeid(IEventBus), m_EventBus);
    RegisterService(typeid(ICommandRegistry), m_CommandRegistry);
}

void ApplicationContext::SyncThemeFromManager() {
    m_ThemeProvider = ThemeManager::Get().SharedProvider();
    m_StyleResolver = ThemeManager::Get().SharedResolver();
    RegisterService(typeid(IThemeProvider), m_ThemeProvider);
    RegisterService(typeid(IStyleResolver), m_StyleResolver);
}

void ApplicationContext::Initialize(float dpiScale) {
    ThemeManager::Get().SetDpiScale(dpiScale);
    SyncThemeFromManager();
    if (m_StyleResolver) {
        m_StyleResolver->SetDpiScale(dpiScale);
    }
    DPIContext::SetScale(dpiScale);
}

void ApplicationContext::Shutdown() {
}

} // namespace WindEffects::Editor::UI
