#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"

#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"
#include "WindEffects/Editor/UI/Resources/ModuleResourceRegistry.h"
#include "WindEffects/Editor/UI/Events/EventBus.h"
#include "WindEffects/Editor/UI/Commands/CommandRegistry.h"
#include "WindEffects/Editor/UI/Docking/DockManager.h"
#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"

#include "Core/DPIContext.h"

namespace WindEffects::Editor::UI {

EditorApplicationContext::EditorApplicationContext() {
    m_ThemeProvider = std::make_shared<GraphiteDarkTheme>();
    m_StyleResolver = std::make_shared<StyleResolver>(m_ThemeProvider);
    m_ResourceRegistry = std::make_shared<ModuleResourceRegistry>();
    m_EventBus = std::make_shared<EventBus>();
    m_CommandRegistry = std::make_shared<CommandRegistry>();
    m_DockManager = std::make_shared<DockManager>();
    m_ExtensionRegistry = std::make_shared<UIExtensionRegistry>();

    RegisterService(typeid(IThemeProvider), m_ThemeProvider);
    RegisterService(typeid(IStyleResolver), m_StyleResolver);
    RegisterService(typeid(IResourceRegistry), m_ResourceRegistry);
    RegisterService(typeid(IEventBus), m_EventBus);
    RegisterService(typeid(ICommandRegistry), m_CommandRegistry);
    RegisterService(typeid(IDockManager), m_DockManager);
    RegisterService(typeid(UIExtensionRegistry), m_ExtensionRegistry);
}

void EditorApplicationContext::Initialize(float dpiScale) {
    m_StyleResolver->SetDpiScale(dpiScale);
    DPIContext::SetScale(dpiScale);
    m_DockManager->SetLayout(CreateDefaultEditorWorkspaceLayout());
    ExtensionBootstrap::Instance().FlushTo(*m_ExtensionRegistry);
    m_ExtensionRegistry->PopulateDockManager(*m_DockManager);
    m_ExtensionRegistry->PopulateCommandRegistry(*m_CommandRegistry);
}

void EditorApplicationContext::Shutdown() {
}

} // namespace WindEffects::Editor::UI


