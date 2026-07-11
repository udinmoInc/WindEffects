#include "Layout/OverlayManager.h"
#include "Core/PaintContext.h"
#include "Core/Logger.h"

#include <algorithm>

namespace WindEffects::Editor::UI {

OverlayHost::OverlayHost() = default;
OverlayHost::~OverlayHost() = default;

void OverlayHost::SetBaseWidget(const std::shared_ptr<Widget>& baseWidget) {
    if (m_BaseWidget) {
        RemoveChild(m_BaseWidget);
    }
    m_BaseWidget = baseWidget;
    if (m_BaseWidget) {
        AddChild(m_BaseWidget);
    }
}

void OverlayHost::ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) {
    const float width = (std::max)(m_Geometry.width, 1.0f);
    const float height = (std::max)(m_Geometry.height, 1.0f);
    Size size = popup->Measure(Size{width, height});
    Rect geom{position.x, position.y, size.width, size.height};
    popup->Arrange(geom);

    m_Popups.push_back(popup);
    m_FullscreenPopups.push_back(false);
    AddChild(popup);
}

void OverlayHost::ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) {
    const float width = (std::max)(m_Geometry.width, 1.0f);
    const float height = (std::max)(m_Geometry.height, 1.0f);
    const Rect geom{0.0f, 0.0f, width, height};
    popup->Measure(Size{width, height});
    popup->Arrange(geom);

    m_Popups.push_back(popup);
    m_FullscreenPopups.push_back(true);
    AddChild(popup);
}

void OverlayHost::CloseTopPopup() {
    if (m_Popups.empty()) {
        return;
    }
    RemoveChild(m_Popups.back());
    m_Popups.pop_back();
    m_FullscreenPopups.pop_back();
}

void OverlayHost::CloseAllPopups() {
    for (auto& popup : m_Popups) {
        RemoveChild(popup);
    }
    m_Popups.clear();
    m_FullscreenPopups.clear();
}

void OverlayHost::ExecutePendingCallbacks() {
    for (auto& popup : m_Popups) {
        if (popup) {
            popup->ExecutePendingCallback();
        }
    }
}

bool OverlayHost::IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const {
    if (!widget) {
        return false;
    }

    for (auto current = widget; current; current = current->GetParent()) {
        for (const auto& popup : m_Popups) {
            if (current == popup) {
                return true;
            }
        }
    }
    return false;
}

Size OverlayHost::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    if (m_BaseWidget) {
        m_BaseWidget->Measure(availableSize);
    }
    return availableSize;
}

void OverlayHost::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_BaseWidget) {
        m_BaseWidget->Arrange(allottedRect);
    }

    for (size_t i = 0; i < m_Popups.size(); ++i) {
        auto& popup = m_Popups[i];
        if (i < m_FullscreenPopups.size() && m_FullscreenPopups[i]) {
            popup->Measure(Size{allottedRect.width, allottedRect.height});
            popup->Arrange(allottedRect);
            continue;
        }

        Rect geom = popup->GetGeometry();
        if (geom.x + geom.width > allottedRect.width) {
            geom.x = allottedRect.width - geom.width;
        }
        if (geom.y + geom.height > allottedRect.height) {
            geom.y = allottedRect.height - geom.height;
        }
        popup->Arrange(geom);
    }
}

void OverlayHost::Paint(PaintContext& context) {
    if (m_BaseWidget) {
        m_BaseWidget->Paint(context);
    }
    for (auto& popup : m_Popups) {
        popup->Paint(context);
    }
}

void OverlayHost::OnMouseDown(const MouseEvent&) {
    CloseAllPopups();
}

} // namespace WindEffects::Editor::UI
