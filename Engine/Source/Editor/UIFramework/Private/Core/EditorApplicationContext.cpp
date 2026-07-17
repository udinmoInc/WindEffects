#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"

#include "WindEffects/Editor/UI/Theming/EditorTheme.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Resources/ModuleResourceRegistry.h"
#include "KindUI/Events/EventBus.h"
#include "KindUI/Commands/CommandRegistry.h"
#include "WindEffects/Editor/UI/Docking/DockManager.h"
#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"

#include "KindUI/Core/DPIContext.h"

using ::we::runtime::kindui::ThemeManager;
using ::we::runtime::kindui::ModuleResourceRegistry;
using ::we::runtime::kindui::EventBus;
using ::we::runtime::kindui::CommandRegistry;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IKindUITheme;
using ::we::runtime::kindui::IStyleResolver;
using ::we::runtime::kindui::IResourceRegistry;
using ::we::runtime::kindui::IEventBus;
using ::we::runtime::kindui::ICommandRegistry;

namespace we::editor::services {
using ::we::editor::docking::DockManager;
using ::we::editor::docking::CreateDefaultEditorWorkspaceLayout;
using ::we::editor::extensions::ExtensionBootstrap;
using ::we::editor::extensions::UIExtensionRegistry;

EditorApplicationContext::EditorApplicationContext(std::shared_ptr<IKindUITheme> theme) {
    if (!theme) {
        theme = std::make_shared<::we::editor::services::EditorTheme>();
    }
    ThemeManager::Get().Initialize(std::move(theme), 1.0f);
    m_ThemeProvider = ThemeManager::Get().SharedTheme();
    m_StyleResolver = ThemeManager::Get().SharedResolver();
    m_ResourceRegistry = std::make_shared<ModuleResourceRegistry>();
    m_EventBus = std::make_shared<EventBus>();
    m_CommandRegistry = std::make_shared<CommandRegistry>();
    m_DockManager = std::make_shared<DockManager>();
    m_ExtensionRegistry = std::make_shared<UIExtensionRegistry>();

    RegisterService(typeid(IKindUITheme), m_ThemeProvider);
    RegisterService(typeid(IStyleResolver), m_StyleResolver);
    RegisterService(typeid(IResourceRegistry), m_ResourceRegistry);
    RegisterService(typeid(IEventBus), m_EventBus);
    RegisterService(typeid(ICommandRegistry), m_CommandRegistry);
    RegisterService(typeid(IDockManager), m_DockManager);
    RegisterService(typeid(UIExtensionRegistry), m_ExtensionRegistry);
}

void EditorApplicationContext::SyncThemeFromManager() {
    m_ThemeProvider = ThemeManager::Get().SharedTheme();
    m_StyleResolver = ThemeManager::Get().SharedResolver();
    RegisterService(typeid(IKindUITheme), m_ThemeProvider);
    RegisterService(typeid(IStyleResolver), m_StyleResolver);
}

void EditorApplicationContext::Initialize(float dpiScale) {
    ThemeManager::Get().SetDpiScale(dpiScale);
    SyncThemeFromManager();
    if (m_StyleResolver) {
        m_StyleResolver->SetDpiScale(dpiScale);
    }
    DPIContext::SetScale(dpiScale);
    m_DockManager->SetLayout(CreateDefaultEditorWorkspaceLayout());
    ExtensionBootstrap::Instance().FlushTo(*m_ExtensionRegistry);
    m_ExtensionRegistry->PopulateDockManager(*m_DockManager);
    m_ExtensionRegistry->PopulateCommandRegistry(*m_CommandRegistry);
}

void EditorApplicationContext::Shutdown() {
}

} // namespace we::editor::services

// relink
