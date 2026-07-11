#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"

#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"
#include "WindEffects/Editor/UI/Resources/ModuleResourceRegistry.h"
#include "WindEffects/Editor/UI/Events/EventBus.h"
#include "WindEffects/Editor/UI/Commands/CommandRegistry.h"
#include "WindEffects/Editor/UI/Docking/DockManager.h"

#include "Core/Theme.h"
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
    RegisterService(typeid(IEditorApplicationContext), std::shared_ptr<void>{});
}

void EditorApplicationContext::Initialize(float dpiScale) {
    m_StyleResolver->SetDpiScale(dpiScale);
    m_DockManager->SetLayout(CreateDefaultEditorWorkspaceLayout());
    SyncLegacyThemeFromProvider(*m_ThemeProvider, dpiScale);
}

void EditorApplicationContext::Shutdown() {
    m_ExtensionRegistry->PopulateCommandRegistry(*m_CommandRegistry);
}

void SyncLegacyThemeFromProvider(const IThemeProvider& theme, float dpiScale) {
    auto& legacy = we::UI::Theme::Get();
    we::UI::DPIContext::SetScale(dpiScale);

    legacy.WindowBackground = {
        theme.GetColor(ThemeToken::WindowBackground).r,
        theme.GetColor(ThemeToken::WindowBackground).g,
        theme.GetColor(ThemeToken::WindowBackground).b,
        theme.GetColor(ThemeToken::WindowBackground).a
    };
    legacy.WorkspaceBackground = {
        theme.GetColor(ThemeToken::WorkspaceBackground).r,
        theme.GetColor(ThemeToken::WorkspaceBackground).g,
        theme.GetColor(ThemeToken::WorkspaceBackground).b,
        theme.GetColor(ThemeToken::WorkspaceBackground).a
    };
    legacy.ToolbarBackground = {
        theme.GetColor(ThemeToken::ToolbarBackground).r,
        theme.GetColor(ThemeToken::ToolbarBackground).g,
        theme.GetColor(ThemeToken::ToolbarBackground).b,
        theme.GetColor(ThemeToken::ToolbarBackground).a
    };
    legacy.PanelBackground = {
        theme.GetColor(ThemeToken::PanelBackground).r,
        theme.GetColor(ThemeToken::PanelBackground).g,
        theme.GetColor(ThemeToken::PanelBackground).b,
        theme.GetColor(ThemeToken::PanelBackground).a
    };
    legacy.HeaderBackground = {
        theme.GetColor(ThemeToken::HeaderBackground).r,
        theme.GetColor(ThemeToken::HeaderBackground).g,
        theme.GetColor(ThemeToken::HeaderBackground).b,
        theme.GetColor(ThemeToken::HeaderBackground).a
    };
    legacy.ViewportBackground = {
        theme.GetColor(ThemeToken::ViewportBackground).r,
        theme.GetColor(ThemeToken::ViewportBackground).g,
        theme.GetColor(ThemeToken::ViewportBackground).b,
        theme.GetColor(ThemeToken::ViewportBackground).a
    };
    legacy.MenuBarBackground = {
        theme.GetColor(ThemeToken::MenuBarBackground).r,
        theme.GetColor(ThemeToken::MenuBarBackground).g,
        theme.GetColor(ThemeToken::MenuBarBackground).b,
        theme.GetColor(ThemeToken::MenuBarBackground).a
    };
    legacy.TabBackground = {
        theme.GetColor(ThemeToken::TabBackground).r,
        theme.GetColor(ThemeToken::TabBackground).g,
        theme.GetColor(ThemeToken::TabBackground).b,
        theme.GetColor(ThemeToken::TabBackground).a
    };
    legacy.StatusBarBackground = {
        theme.GetColor(ThemeToken::StatusBarBackground).r,
        theme.GetColor(ThemeToken::StatusBarBackground).g,
        theme.GetColor(ThemeToken::StatusBarBackground).b,
        theme.GetColor(ThemeToken::StatusBarBackground).a
    };
    legacy.TextPrimary = {
        theme.GetColor(ThemeToken::TextPrimary).r,
        theme.GetColor(ThemeToken::TextPrimary).g,
        theme.GetColor(ThemeToken::TextPrimary).b,
        theme.GetColor(ThemeToken::TextPrimary).a
    };
    legacy.TextSecondary = {
        theme.GetColor(ThemeToken::TextSecondary).r,
        theme.GetColor(ThemeToken::TextSecondary).g,
        theme.GetColor(ThemeToken::TextSecondary).b,
        theme.GetColor(ThemeToken::TextSecondary).a
    };
    legacy.IconDefault = {
        theme.GetColor(ThemeToken::IconDefault).r,
        theme.GetColor(ThemeToken::IconDefault).g,
        theme.GetColor(ThemeToken::IconDefault).b,
        theme.GetColor(ThemeToken::IconDefault).a
    };
    legacy.IconHover = {
        theme.GetColor(ThemeToken::IconHover).r,
        theme.GetColor(ThemeToken::IconHover).g,
        theme.GetColor(ThemeToken::IconHover).b,
        theme.GetColor(ThemeToken::IconHover).a
    };
    legacy.AccentPrimary = {
        theme.GetColor(ThemeToken::AccentPrimary).r,
        theme.GetColor(ThemeToken::AccentPrimary).g,
        theme.GetColor(ThemeToken::AccentPrimary).b,
        theme.GetColor(ThemeToken::AccentPrimary).a
    };
    legacy.ActiveTabLine = {
        theme.GetColor(ThemeToken::ActiveTabLine).r,
        theme.GetColor(ThemeToken::ActiveTabLine).g,
        theme.GetColor(ThemeToken::ActiveTabLine).b,
        theme.GetColor(ThemeToken::ActiveTabLine).a
    };
    legacy.BorderDefault = {
        theme.GetColor(ThemeToken::BorderDefault).r,
        theme.GetColor(ThemeToken::BorderDefault).g,
        theme.GetColor(ThemeToken::BorderDefault).b,
        theme.GetColor(ThemeToken::BorderDefault).a
    };
    legacy.HoverButton = {
        theme.GetColor(ThemeToken::HoverBackground).r,
        theme.GetColor(ThemeToken::HoverBackground).g,
        theme.GetColor(ThemeToken::HoverBackground).b,
        theme.GetColor(ThemeToken::HoverBackground).a
    };
    legacy.SelectedBg = {
        theme.GetColor(ThemeToken::SelectedBackground).r,
        theme.GetColor(ThemeToken::SelectedBackground).g,
        theme.GetColor(ThemeToken::SelectedBackground).b,
        theme.GetColor(ThemeToken::SelectedBackground).a
    };

    legacy.PanelHeaderHeight = theme.GetMetric(ThemeToken::PanelHeaderHeight);
    legacy.ToolbarHeight = theme.GetMetric(ThemeToken::ToolbarHeight);
    legacy.SearchBoxHeight = theme.GetMetric(ThemeToken::SearchBoxHeight);
    legacy.IconButtonSize = theme.GetMetric(ThemeToken::IconButtonSize);
    legacy.ButtonHeight = theme.GetMetric(ThemeToken::ButtonHeight);
    legacy.IconSizeToolbar = theme.GetMetric(ThemeToken::IconSizeToolbar);
    legacy.CornerRadiusSmall = theme.GetMetric(ThemeToken::CornerRadiusSmall);
    legacy.CornerRadiusMedium = theme.GetMetric(ThemeToken::CornerRadiusMedium);
    legacy.TextSizeMenu = theme.GetMetric(ThemeToken::TextSizeMenu);
    legacy.TextSizeToolbar = theme.GetMetric(ThemeToken::TextSizeToolbar);
    legacy.TextSizeTabs = theme.GetMetric(ThemeToken::TextSizeTabs);
    legacy.TextSizeNormal = theme.GetMetric(ThemeToken::TextSizeNormal);
    legacy.TextSizeCaption = theme.GetMetric(ThemeToken::TextSizeCaption);
}

} // namespace WindEffects::Editor::UI
