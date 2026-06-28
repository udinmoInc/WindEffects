#include "Core/EventSystem.hpp"
#include "Core/Widget.hpp"

namespace we::UI {

std::shared_ptr<Widget> EventSystem::HitTest(const std::shared_ptr<Widget>& root, const Point& pos) {
    if (!root || !root->IsVisible()) return nullptr;
    if (!root->GetGeometry().Contains(pos)) return nullptr;

    // Search children in reverse order (top-most widgets drawn last)
    const auto& children = root->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto hit = HitTest(*it, pos);
        if (hit) return hit;
    }

    return root;
}

void EventSystem::ProcessMouseEvent(const MouseEvent& event) {
    if (!m_Root) return;

    // Find widget under mouse
    std::shared_ptr<Widget> hitWidget = HitTest(m_Root, event.position);

    // Handle Hover changes
    std::shared_ptr<Widget> oldHovered = m_HoveredWidget.lock();
    if (hitWidget != oldHovered) {
        if (oldHovered) {
            oldHovered->SetHovered(false);
        }
        if (hitWidget) {
            hitWidget->SetHovered(true);
        }
        m_HoveredWidget = hitWidget;
    }

    // Process mouse actions
    if (hitWidget) {
        if (event.type == MouseEventType::MouseDown) {
            SetFocusedWidget(hitWidget);
            hitWidget->OnMouseDown(event);
        } else if (event.type == MouseEventType::MouseUp) {
            hitWidget->OnMouseUp(event);
        } else if (event.type == MouseEventType::MouseMove) {
            hitWidget->OnMouseMove(event);
        } else if (event.type == MouseEventType::MouseWheel) {
            hitWidget->OnMouseWheel(event);
        }
    } else {
        if (event.type == MouseEventType::MouseDown) {
            SetFocusedWidget(nullptr);
        }
    }
}

void EventSystem::ProcessKeyEvent(const KeyEvent& event) {
    if (auto focused = m_FocusedWidget.lock()) {
        focused->OnKeyDown(event);
    }
}

void EventSystem::SetFocusedWidget(const std::shared_ptr<Widget>& widget) {
    std::shared_ptr<Widget> oldFocused = m_FocusedWidget.lock();
    if (widget == oldFocused) return;

    if (oldFocused) {
        oldFocused->OnBlur();
    }
    if (widget) {
        widget->OnFocus();
    }
    m_FocusedWidget = widget;
}

} // namespace we::editor::application::UI
