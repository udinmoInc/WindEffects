// ==============================================================================
// WindEffects — KindUI — PopupService
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "App/PopupService.h"

#include "KindUI/Compose/ViewBuilder.h"
#include "KindUI/UI/OverlayManager.h"
#include "KindUI/UI/PopupPositioner.h"

namespace we::runtime::kindui {

PopupService::PopupService(IPopupHost* host, std::shared_ptr<IWidgetContext> context)
    : m_Host(host)
    , m_Context(std::move(context)) {}

void PopupService::ShowMenu(Element menu, Point position) {
    if (!m_Host) {
        return;
    }
    if (auto popup = BuildMenu(std::move(menu))) {
        // Shared placement: measure → prefer below point → flip/clamp inside host.
        m_Host->ShowPopup(popup, position);
    }
}

void PopupService::ShowMenuAt(Element menu, const Widget& anchor) {
    if (!m_Host) {
        return;
    }
    if (auto popup = BuildMenu(std::move(menu))) {
        // Prefer live tracking when the host is the concrete OverlayHost.
        if (auto* overlay = dynamic_cast<OverlayHost*>(m_Host)) {
            if (auto live = const_cast<Widget&>(anchor).weak_from_this().lock()) {
                overlay->ShowAnchoredPopup(popup, live, PopupPlacementMode::BottomPreferred);
                return;
            }
        }
        m_Host->ShowAnchoredPopup(
            popup, anchor.GetGeometry(), PopupPlacementMode::BottomPreferred);
    }
}

void PopupService::DismissTop() {
    if (m_Host) {
        m_Host->CloseTopPopup();
    }
}

void PopupService::DismissAll() {
    if (m_Host) {
        m_Host->CloseAllPopups();
    }
}

bool PopupService::HasOpenPopups() const {
    return m_Host && m_Host->HasOpenPopups();
}

std::shared_ptr<Widget> PopupService::BuildMenu(const Element& menu) {
    if (!m_Context) {
        return nullptr;
    }
    ViewBuilder builder(m_Context);
    return builder.Build(menu);
}

} // namespace we::runtime::kindui
