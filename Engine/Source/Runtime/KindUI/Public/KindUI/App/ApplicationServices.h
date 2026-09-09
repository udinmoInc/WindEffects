// ==============================================================================
// WindEffects — KindUI — ApplicationServices
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/App/DialogService.h"
#include "KindUI/App/PopupService.h"
#include "KindUI/Export.h"
#include "KindUI/Layout/IPopupHost.h"

#include <memory>

namespace we::runtime::kindui {

/// Bundled application infrastructure services for KindUI applications.
struct KINDUI_API ApplicationServices {
    std::shared_ptr<IWidgetContext> context;
    IPopupHost* popupHost = nullptr;

    std::unique_ptr<DialogService> dialogs;
    std::unique_ptr<PopupService> popups;

    void Initialize(std::shared_ptr<IWidgetContext> widgetContext, IPopupHost* host = nullptr);
};

} // namespace we::runtime::kindui
