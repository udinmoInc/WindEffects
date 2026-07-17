#include "KindUI/Core/WidgetContext.h"

namespace we::runtime::kindui {

WidgetContext::WidgetContext(IApplicationContext& appContext, IPopupHost* popupHost)
    : m_AppContext(appContext)
    , m_PopupHost(popupHost) {}

IStyleResolver& WidgetContext::GetStyleResolver() const {
    return m_AppContext.GetStyleResolver();
}

IThemeProvider& WidgetContext::GetThemeProvider() const {
    return m_AppContext.GetThemeProvider();
}

IEventBus& WidgetContext::GetEventBus() const {
    return m_AppContext.GetEventBus();
}

ICommandRegistry& WidgetContext::GetCommandRegistry() const {
    return m_AppContext.GetCommandRegistry();
}

IResourceRegistry& WidgetContext::GetResourceRegistry() const {
    return m_AppContext.GetResourceRegistry();
}

} // namespace we::runtime::kindui
