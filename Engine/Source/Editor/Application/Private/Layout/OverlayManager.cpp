#include "Layout/OverlayManager.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Logger.hpp"

namespace we::UI {

OverlayManager* OverlayManager::s_Instance = nullptr;

OverlayManager::OverlayManager() {
    s_Instance = this;
}

OverlayManager::~OverlayManager() {
    if (s_Instance == this) {
        s_Instance = nullptr;
    }
}

OverlayManager* OverlayManager::Get() {
    return s_Instance;
}

void OverlayManager::SetBaseWidget(const std::shared_ptr<Widget>& baseWidget) {
    if (m_BaseWidget) {
        RemoveChild(m_BaseWidget);
    }
    m_BaseWidget = baseWidget;
    if (m_BaseWidget) {
        AddChild(m_BaseWidget);
    }
}

void OverlayManager::ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) {
    // Determine required size
    Size size = popup->Measure(Size{ 10000.0f, 10000.0f }); // Arbitrary large size
    
    // Position it
    Rect geom{ position.x, position.y, size.width, size.height };
    popup->Arrange(geom);
    
    m_Popups.push_back(popup);
    AddChild(popup);
}

void OverlayManager::CloseTopPopup() {
    if (!m_Popups.empty()) {
        auto popup = m_Popups.back();
        RemoveChild(popup);
        m_Popups.pop_back();
    }
}

void OverlayManager::CloseAllPopups() {
    for (auto& popup : m_Popups) {
        RemoveChild(popup);
    }
    m_Popups.clear();
}

void OverlayManager::ExecutePendingCallbacks() {
    // Execute pending callbacks for all popups
    // This is called after event processing to avoid use-after-free
    try {
        // Skip if no popups or during initialization
        if (m_Popups.empty()) {
            return;
        }
        
        for (auto& popup : m_Popups) {
            if (popup) {
                popup->ExecutePendingCallback();
            }
        }
    } catch (const std::exception& e) {
        HE_ERROR("Error in OverlayManager::ExecutePendingCallbacks: " + std::string(e.what()));
    } catch (...) {
        HE_ERROR("Unknown error in OverlayManager::ExecutePendingCallbacks");
    }
}

bool OverlayManager::IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const {
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

Size OverlayManager::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    if (m_BaseWidget) {
        m_BaseWidget->Measure(availableSize);
    }
    return availableSize;
}

void OverlayManager::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_BaseWidget) {
        m_BaseWidget->Arrange(allottedRect);
    }
    
    // Re-arrange popups if necessary to keep them on screen
    for (auto& popup : m_Popups) {
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

void OverlayManager::Paint(PaintContext& context) {
    if (m_BaseWidget) {
        m_BaseWidget->Paint(context);
    }
    for (auto& popup : m_Popups) {
        popup->Paint(context);
    }
}

void OverlayManager::OnMouseDown(const MouseEvent& /*event*/) {
    // If a click happens and it doesn't hit a popup, close all popups.
    // EventSystem route mouse events by hitting the root.
    // If it hit the background of OverlayManager (meaning no popup hit and maybe no base hit),
    // or if we intercept a click...
    // Actually, EventSystem calls OnMouseDown on the exact hit widget. 
    // OverlayManager only gets OnMouseDown if the click didn't hit baseWidget or popups.
    CloseAllPopups();
}

} // namespace we::editor::application::UI
