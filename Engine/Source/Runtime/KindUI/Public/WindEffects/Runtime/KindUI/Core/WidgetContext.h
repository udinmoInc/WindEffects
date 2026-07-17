#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/IApplicationContext.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Events/IEventBus.h"
#include "WindEffects/Runtime/UI/Commands/ICommandRegistry.h"
#include "WindEffects/Runtime/UI/Resources/IResourceRegistry.h"
#include "WindEffects/Runtime/UI/Layout/IPopupHost.h"

#include <memory>

namespace WindEffects::Editor::UI {

class UI_API IWidgetContext {
public:
    virtual ~IWidgetContext() = default;

    [[nodiscard]] virtual IApplicationContext& GetApplicationContext() const = 0;
    [[nodiscard]] virtual IStyleResolver& GetStyleResolver() const = 0;
    [[nodiscard]] virtual IThemeProvider& GetThemeProvider() const = 0;
    [[nodiscard]] virtual IEventBus& GetEventBus() const = 0;
    [[nodiscard]] virtual ICommandRegistry& GetCommandRegistry() const = 0;
    [[nodiscard]] virtual IResourceRegistry& GetResourceRegistry() const = 0;
    [[nodiscard]] virtual IPopupHost* GetPopupHost() const = 0;
};

class UI_API WidgetContext final : public IWidgetContext {
public:
    explicit WidgetContext(IApplicationContext& appContext, IPopupHost* popupHost = nullptr);

    [[nodiscard]] IApplicationContext& GetApplicationContext() const override { return m_AppContext; }
    [[nodiscard]] IStyleResolver& GetStyleResolver() const override;
    [[nodiscard]] IThemeProvider& GetThemeProvider() const override;
    [[nodiscard]] IEventBus& GetEventBus() const override;
    [[nodiscard]] ICommandRegistry& GetCommandRegistry() const override;
    [[nodiscard]] IResourceRegistry& GetResourceRegistry() const override;
    [[nodiscard]] IPopupHost* GetPopupHost() const override { return m_PopupHost; }

    void SetPopupHost(IPopupHost* popupHost) { m_PopupHost = popupHost; }

private:
    IApplicationContext& m_AppContext;
    IPopupHost* m_PopupHost = nullptr;
};

} // namespace WindEffects::Editor::UI
