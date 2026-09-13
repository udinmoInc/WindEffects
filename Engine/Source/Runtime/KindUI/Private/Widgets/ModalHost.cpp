// ==============================================================================
// WindEffects — KindUI — ModalHost
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/ModalHost.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

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
    float posX = allottedRect.x + (allottedRect.width - w) * 0.5f;
    float posY = allottedRect.y + (allottedRect.height - h) * 0.5f;

    if (m_AnchorPosition && !m_CenterInParent) {
        posX = std::clamp(m_AnchorPosition->x, allottedRect.x + 8.0f, (std::max)(allottedRect.x + 8.0f,
            allottedRect.x + allottedRect.width - w - 8.0f));
        posY = std::clamp(m_AnchorPosition->y, allottedRect.y + 8.0f, (std::max)(allottedRect.y + 8.0f,
            allottedRect.y + allottedRect.height - h - 8.0f));
    }

    m_Content->Arrange(Rect{ posX, posY, w, h });
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

