// ==============================================================================
// WindEffects — KindUI — ApplicationServices
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/App/ApplicationServices.h"

namespace we::runtime::kindui {

void ApplicationServices::Initialize(std::shared_ptr<IWidgetContext> widgetContext, IPopupHost* host) {
    context = std::move(widgetContext);
    popupHost = host;
    dialogs = std::make_unique<DialogService>(context);
    popups = std::make_unique<PopupService>(host, context);
}

} // namespace we::runtime::kindui
 
