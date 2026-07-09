#include "Core/Widget.h"
#include <algorithm>

namespace we::UI {

Widget::Diagnostics* Widget::s_GlobalDiagnostics = nullptr;

// Legacy static counters for compatibility (deprecated - use s_GlobalDiagnostics instead)
uint32_t Widget::s_TotalWidgetCount = 0;
uint32_t Widget::s_VisibleWidgetCount = 0;
uint32_t Widget::s_HiddenWidgetCount = 0;
uint32_t Widget::s_PaintCalls = 0;
uint32_t Widget::s_ArrangeChildrenCalls = 0;
uint32_t Widget::s_LayoutPassCount = 0;
uint32_t Widget::s_InvalidateCount = 0;
uint32_t Widget::s_WidgetsPainted = 0;

void Widget::ResetDiagnostics() {
    if (s_GlobalDiagnostics) {
        s_GlobalDiagnostics->Reset();
    }
    // Legacy static reset for compatibility
    s_TotalWidgetCount = 0;
    s_VisibleWidgetCount = 0;
    s_HiddenWidgetCount = 0;
    s_PaintCalls = 0;
    s_ArrangeChildrenCalls = 0;
    s_LayoutPassCount = 0;
    s_InvalidateCount = 0;
    s_WidgetsPainted = 0;
}

void Widget::Tick(float deltaTime) {
    if (!m_Visible) return;
    
    // We create a copy of children in case they modify the child list during Tick
    auto childrenCopy = m_Children;
    for (auto& child : childrenCopy) {
        if (child) {
            child->Tick(deltaTime);
        }
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
