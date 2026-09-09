// ==============================================================================
// WindEffects — KindUI — WidgetContext
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/WidgetContext.h"

namespace we::runtime::kindui {

WidgetContext::WidgetContext(IApplicationContext& appContext, IPopupHost* popupHost)
    : m_AppContext(appContext)
    , m_PopupHost(popupHost) {}

IStyleResolver& WidgetContext::GetStyleResolver() const {
    return m_AppContext.GetStyleResolver();
}

IKindUITheme& WidgetContext::GetTheme() const {
    return m_AppContext.GetTheme();
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
 
