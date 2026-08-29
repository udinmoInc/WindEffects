#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Core/Logger.h"

#include <algorithm>

namespace we::runtime::kindui {

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
    const float screenW = (std::max)(m_Geometry.width, 1.0f);
    const float screenH = (std::max)(m_Geometry.height, 1.0f);
    const float margin = ResolveMetric(MetricToken::Space1);
    const float screenMargin = ResolveMetric(MetricToken::Space4);
    const float maxW = (std::max)(ResolveMetric(MetricToken::PopupMinWidth) * 0.35f, screenW - screenMargin);
    const float availBelowY = (std::max)(ResolveMetric(MetricToken::PopupMinWidth) * 0.35f, screenH - position.y - margin);

    Size size = popup->Measure(Size{ maxW, availBelowY });
    size = popup->ClampDesiredSize(size);

    float posX = position.x;
    float posY = position.y;

    const float flipOffset = ResolveMetric(MetricToken::PanelToolbarHeight);
    if (posY + size.height > screenH - margin && posY - size.height >= margin) {
        posY = posY - size.height - flipOffset;
    }
    posX = std::clamp(posX, margin, (std::max)(margin, screenW - size.width - margin));
    posY = std::clamp(posY, margin, (std::max)(margin, screenH - size.height - margin));

    Rect geom{ posX, posY, size.width, size.height };
    popup->Arrange(geom);

    m_Popups.push_back(popup);
    m_FullscreenPopups.push_back(false);
    m_PopupCachedSizes.push_back(size);
    AttachOverlayChild(popup);
}

void OverlayHost::ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) {
    const float width = (std::max)(m_Geometry.width, 1.0f);
    const float height = (std::max)(m_Geometry.height, 1.0f);
    const Rect geom{0.0f, 0.0f, width, height};
    popup->Measure(Size{width, height});
    popup->Arrange(geom);

    m_Popups.push_back(popup);
    m_FullscreenPopups.push_back(true);
    m_PopupCachedSizes.push_back(Size{width, height});
    AttachOverlayChild(popup);
}

void OverlayHost::CloseTopPopup() {
    if (m_Popups.empty()) {
        return;
    }
    DetachOverlayChild(m_Popups.back());
    m_Popups.pop_back();
    m_FullscreenPopups.pop_back();
    if (!m_PopupCachedSizes.empty()) {
        m_PopupCachedSizes.pop_back();
    }
}

void OverlayHost::CloseAllPopups() {
    for (auto& popup : m_Popups) {
        DetachOverlayChild(popup);
    }
    m_Popups.clear();
    m_FullscreenPopups.clear();
    m_PopupCachedSizes.clear();
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
    const bool hostResized =
        std::abs(allottedRect.width - m_LastArrangeSize.width) > 0.5f
        || std::abs(allottedRect.height - m_LastArrangeSize.height) > 0.5f;
    m_LastArrangeSize = Size{allottedRect.width, allottedRect.height};
    ClearLayoutDirty();
    if (m_BaseWidget) {
        m_BaseWidget->Arrange(allottedRect);
    }

    const float popupMargin = ResolveMetric(MetricToken::Space2) * 2.0f;
    const float minPopupDim = ResolveMetric(MetricToken::ToolbarLabeledMinWidth) + ResolveMetric(MetricToken::Space2);
    const float maxW = (std::max)(minPopupDim, allottedRect.width - popupMargin);

    for (size_t i = 0; i < m_Popups.size(); ++i) {
        auto& popup = m_Popups[i];
        if (i < m_FullscreenPopups.size() && m_FullscreenPopups[i]) {
            if (hostResized || popup->NeedsLayout()) {
                popup->Measure(Size{allottedRect.width, allottedRect.height});
            }
            popup->Arrange(allottedRect);
            if (i < m_PopupCachedSizes.size()) {
                m_PopupCachedSizes[i] = Size{allottedRect.width, allottedRect.height};
            }
            continue;
        }

        Rect geom = popup->GetGeometry();
        Size size = (i < m_PopupCachedSizes.size()) ? m_PopupCachedSizes[i] : Size{};
        const bool needsRemeasure =
            popup->NeedsLayout() || size.width <= 0.0f || size.height <= 0.0f;
        if (needsRemeasure) {
            const float availH = (std::max)(minPopupDim, allottedRect.height - geom.y - ResolveMetric(MetricToken::Space2));
            size = popup->Measure(Size{maxW, availH});
            size = popup->ClampDesiredSize(size);
            if (i < m_PopupCachedSizes.size()) {
                m_PopupCachedSizes[i] = size;
            } else {
                m_PopupCachedSizes.push_back(size);
            }
        }

        geom.width = size.width;
        geom.height = size.height;

        if (geom.x + geom.width > allottedRect.width - 4.0f) {
            geom.x = (std::max)(4.0f, allottedRect.width - geom.width - 4.0f);
        }
        if (geom.y + geom.height > allottedRect.height - 4.0f) {
            geom.y = (std::max)(4.0f, allottedRect.height - geom.height - 4.0f);
        }
        if (geom.x < 4.0f) geom.x = 4.0f;
        if (geom.y < 4.0f) geom.y = 4.0f;

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
    // Popup dismissal is handled by EventSystem when clicking outside a popup.
    // Do not close here — empty-area hits on this host must not swallow clicks.
}

std::shared_ptr<Widget> OverlayHost::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

    for (auto it = m_Popups.rbegin(); it != m_Popups.rend(); ++it) {
        if (!*it) {
            continue;
        }
        if (auto hit = (*it)->HitTestPoint(pos, clip)) {
            return hit;
        }
    }

    if (m_BaseWidget) {
        return m_BaseWidget->HitTestPoint(pos, clip);
    }

    return nullptr;
}

} // namespace we::runtime::kindui
