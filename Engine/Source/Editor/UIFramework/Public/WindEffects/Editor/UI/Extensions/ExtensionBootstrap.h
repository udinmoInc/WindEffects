#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "KindUI/Commands/LambdaCommand.h"

namespace we::editor::ui {
using we::runtime::kindui::ICommand;

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
                we::editor::ui::PanelRegistration _reg{}; \
                _reg.descriptor = (DescriptorExpr); \
                _reg.descriptor.id = #PanelId; \
                _reg.factory = (FactoryFunc); \
                we::editor::ui::ExtensionBootstrap::Instance().RegisterPanel(std::move(_reg)); \
            } \
        }; \
        static UiPanelBootstrap_##PanelId g_UiPanelBootstrap_##PanelId; \
    }

#define REGISTER_UI_MENU(MenuName, FactoryFunc, SortOrder) \
    namespace { \
        struct UiMenuBootstrap_##MenuName { \
            UiMenuBootstrap_##MenuName() { \
                we::editor::ui::ExtensionBootstrap::Instance().RegisterMenu( \
                    { #MenuName, (FactoryFunc), (SortOrder) }); \
            } \
        }; \
        static UiMenuBootstrap_##MenuName g_UiMenuBootstrap_##MenuName; \
    }

#define REGISTER_COMMAND(CommandIdStr, DisplayName, ExecuteFunc) \
    namespace { \
        struct UiCommandBootstrap_##__LINE__ { \
            UiCommandBootstrap_##__LINE__() { \
                we::editor::ui::ExtensionBootstrap::Instance().RegisterCommand( \
                    we::editor::ui::MakeCommand(CommandIdStr, DisplayName, ExecuteFunc)); \
            } \
        }; \
        static UiCommandBootstrap_##__LINE__ g_UiCommandBootstrap_##__LINE__; \
    }

} // namespace we::editor::ui
