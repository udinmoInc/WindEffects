#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "WindEffects/Editor/UI/Commands/LambdaCommand.h"

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API ExtensionBootstrap {
public:
    static ExtensionBootstrap& Instance();

    void RegisterPanel(PanelRegistration registration);
    void RegisterMenu(MenuRegistration registration);
    void RegisterCommand(std::shared_ptr<ICommand> command);
    void FlushTo(UIExtensionRegistry& registry);

private:
    ExtensionBootstrap() = default;

#pragma warning(push)
#pragma warning(disable: 4251)
    std::vector<PanelRegistration> m_PendingPanels;
    std::vector<MenuRegistration> m_PendingMenus;
    std::vector<std::shared_ptr<ICommand>> m_PendingCommands;
#pragma warning(pop)
};

#define REGISTER_UI_PANEL(PanelId, DescriptorExpr, FactoryFunc) \
    namespace { \
        struct UiPanelBootstrap_##PanelId { \
            UiPanelBootstrap_##PanelId() { \
                WindEffects::Editor::UI::PanelRegistration _reg{}; \
                _reg.descriptor = (DescriptorExpr); \
                _reg.descriptor.id = #PanelId; \
                _reg.factory = (FactoryFunc); \
                WindEffects::Editor::UI::ExtensionBootstrap::Instance().RegisterPanel(std::move(_reg)); \
            } \
        }; \
        static UiPanelBootstrap_##PanelId g_UiPanelBootstrap_##PanelId; \
    }

#define REGISTER_UI_MENU(MenuName, FactoryFunc, SortOrder) \
    namespace { \
        struct UiMenuBootstrap_##MenuName { \
            UiMenuBootstrap_##MenuName() { \
                WindEffects::Editor::UI::ExtensionBootstrap::Instance().RegisterMenu( \
                    { #MenuName, (FactoryFunc), (SortOrder) }); \
            } \
        }; \
        static UiMenuBootstrap_##MenuName g_UiMenuBootstrap_##MenuName; \
    }

#define REGISTER_COMMAND(CommandId, DisplayName, ExecuteFunc) \
    namespace { \
        struct UiCommandBootstrap_##CommandId { \
            UiCommandBootstrap_##CommandId() { \
                WindEffects::Editor::UI::ExtensionBootstrap::Instance().RegisterCommand( \
                    WindEffects::Editor::UI::MakeCommand(#CommandId, DisplayName, ExecuteFunc)); \
            } \
        }; \
        static UiCommandBootstrap_##CommandId g_UiCommandBootstrap_##CommandId; \
    }

} // namespace WindEffects::Editor::UI
