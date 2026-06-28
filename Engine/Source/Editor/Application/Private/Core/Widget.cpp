#include "Core/Widget.hpp"
#include <algorithm>

namespace we::UI {

void Widget::Tick(float deltaTime) {
    if (!m_Visible) return;
    
    // We create a copy of children in case they modify the child list during Tick
    auto childrenCopy = m_Children;
    for (auto& child : childrenCopy) {
        child->Tick(deltaTime);
    }
}

void Widget::AddChild(const std::shared_ptr<Widget>& child) {
    if (!child) return;

    // Remove from old parent first
    if (auto oldParent = child->GetParent()) {
        oldParent->RemoveChild(child);
    }

    child->m_Parent = shared_from_this();
    m_Children.push_back(child);
}

void Widget::RemoveChild(const std::shared_ptr<Widget>& child) {
    if (!child) return;

    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        child->m_Parent.reset();
        m_Children.erase(it);
    }
}

void Widget::ClearChildren() {
    for (auto& child : m_Children) {
        child->m_Parent.reset();
    }
    m_Children.clear();
}

} // namespace we::editor::application::UI
