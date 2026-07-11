#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Theming/IThemeProvider.h"
#include "WindEffects/Editor/UI/Events/IEventBus.h"
#include "WindEffects/Editor/UI/Commands/ICommandRegistry.h"
#include "WindEffects/Editor/UI/Resources/IResourceRegistry.h"
#include "WindEffects/Editor/UI/Layout/IPopupHost.h"

#include <memory>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API IWidgetContext {
public:
    virtual ~IWidgetContext() = default;

    [[nodiscard]] virtual IEditorApplicationContext& GetApplicationContext() const = 0;
    [[nodiscard]] virtual IStyleResolver& GetStyleResolver() const = 0;
    [[nodiscard]] virtual IThemeProvider& GetThemeProvider() const = 0;
    [[nodiscard]] virtual IEventBus& GetEventBus() const = 0;
    [[nodiscard]] virtual ICommandRegistry& GetCommandRegistry() const = 0;
    [[nodiscard]] virtual IResourceRegistry& GetResourceRegistry() const = 0;
    [[nodiscard]] virtual IPopupHost* GetPopupHost() const = 0;
};

class UIFRAMEWORK_API WidgetContext final : public IWidgetContext {
public:
    explicit WidgetContext(IEditorApplicationContext& appContext, IPopupHost* popupHost = nullptr);

    [[nodiscard]] IEditorApplicationContext& GetApplicationContext() const override { return m_AppContext; }
    [[nodiscard]] IStyleResolver& GetStyleResolver() const override;
    [[nodiscard]] IThemeProvider& GetThemeProvider() const override;
    [[nodiscard]] IEventBus& GetEventBus() const override;
    [[nodiscard]] ICommandRegistry& GetCommandRegistry() const override;
    [[nodiscard]] IResourceRegistry& GetResourceRegistry() const override;
    [[nodiscard]] IPopupHost* GetPopupHost() const override { return m_PopupHost; }

private:
    IEditorApplicationContext& m_AppContext;
    IPopupHost* m_PopupHost = nullptr;
};

} // namespace WindEffects::Editor::UI
