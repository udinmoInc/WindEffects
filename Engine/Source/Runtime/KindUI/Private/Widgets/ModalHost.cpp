// ==============================================================================
// WindEffects — KindUI — ModalHost
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/ModalHost.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/StyleRole.h"
#include "KindUI/UI/PopupPositioner.h"

#include <algorithm>

namespace we::runtime::kindui {

std::shared_ptr<ModalHost> MakeModalHost() {
    return std::make_shared<ModalHost>();
}

void ModalHost::SetContent(const std::shared_ptr<Widget>& content) {
    ClearChildren();
    m_Content = content;
    if (m_Content) {
        AddChild(m_Content);
    }
    InvalidateLayout();
}

bool ModalHost::IsPointerTransparent() const {
    // Empty or hidden modals must not sit on top of the shell and eat clicks.
    return !IsVisible() || !m_Content;
}

Size ModalHost::Measure(const Size& availableSize) {
    if (m_Content) {
        const float scale = DPIContext::GetScale();
        const float measureH = m_DialogHeight > 0.0f
            ? m_DialogHeight * scale
            : availableSize.height * 0.85f;
        m_Content->Measure(Size{ m_DialogWidth * scale, measureH });
    }
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void ModalHost::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (!m_Content) {
        return;
    }
    const float scale = DPIContext::GetScale();
    const float maxW = std::min(m_DialogWidth * scale, allottedRect.width * 0.96f);
    Size desired = m_Content->GetDesiredSize();
    float w = maxW;
    float h = desired.height;
    if (m_DialogHeight > 0.0f) {
        h = std::min(m_DialogHeight * scale, allottedRect.height * 0.94f);
        w = std::min(m_DialogWidth * scale, allottedRect.width * 0.96f);
    } else {
        h = std::min(std::max(desired.height, 200.0f * scale), allottedRect.height * 0.90f);
    }

    PopupPlacementOptions options{};
    options.mode = PopupPlacementMode::AtPoint;
    options.gap = 0.0f;
    options.viewportMargin = 8.0f;
    options.viewportBounds = allottedRect;

    if (m_AnchorPosition && !m_CenterInParent) {
        options.anchorRect = Rect{ m_AnchorPosition->x, m_AnchorPosition->y, 0.0f, 0.0f };
    } else {
        options.anchorRect = Rect{
            allottedRect.x + (allottedRect.width - w) * 0.5f,
            allottedRect.y + (allottedRect.height - h) * 0.5f,
            0.0f,
            0.0f
        };
    }

    const PopupPlacementResult placement = PopupPositioner::Calculate(Size{ w, h }, options);
    m_Content->Arrange(placement.popupRect);
}

void ModalHost::Paint(PaintContext& context) {
    if (m_ShowScrim) {
        const Color scrim = ResolveColor(ColorToken::ModalScrim);
        context.DrawRect(m_Geometry, scrim);
    }
    if (m_Content && m_Content->IsVisible()) {
        m_Content->PaintSubtree(context);
    }
}

void ModalHost::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_DismissOnScrim && m_Content
        && !m_Content->GetGeometry().Contains(event.position)) {
        if (m_OnScrimClicked) {
            m_OnScrimClicked();
        }
    }
}

void ModalHost::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

}

