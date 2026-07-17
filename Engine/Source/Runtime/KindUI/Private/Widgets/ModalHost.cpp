#include "KindUI/Widgets/ModalHost.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeToken.h"

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
    m_Content->Arrange(Rect{
        allottedRect.x + (allottedRect.width - w) * 0.5f,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        w,
        h
    });
}

void ModalHost::Paint(PaintContext& context) {
    Color scrim = ResolveThemeColor(ThemeToken::ModalScrim);
    if (scrim.a < 0.40f || scrim.a > 0.50f) {
        scrim = Color{ 0.0f, 0.0f, 0.0f, 0.45f };
    }
    context.DrawRect(m_Geometry, scrim);
    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
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

} // namespace we::runtime::kindui
