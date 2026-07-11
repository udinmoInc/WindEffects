#include "Core/Widget.h"
#include "WindEffects/Editor/UI/Core/WidgetContext.h"
#include "WindEffects/Editor/UI/Layout/IPopupHost.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

#include <algorithm>
#include <stdexcept>

namespace WindEffects::Editor::UI {

Widget::Diagnostics* Widget::s_GlobalDiagnostics = nullptr;

void Widget::ResetDiagnostics() {
    if (s_GlobalDiagnostics) {
        s_GlobalDiagnostics->Reset();
    }
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
    if (m_Context) {
        child->SetContext(m_Context);
    }
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

void Widget::SetContext(std::shared_ptr<IWidgetContext> context) {
    m_Context = std::move(context);
    for (auto& child : m_Children) {
        if (child) {
            child->SetContext(m_Context);
        }
    }
}

IStyleResolver& Widget::Styles() const {
    if (!m_Context) {
        throw std::runtime_error("Widget accessed style system without context");
    }
    return m_Context->GetStyleResolver();
}

Color Widget::ThemeColor(ThemeToken token) const {
    if (!m_Context) {
        return ResolveThemeColor(token);
    }
    return m_Context->GetThemeProvider().GetColor(token);
}

float Widget::ThemeMetric(ThemeToken token) const {
    if (!m_Context) {
        return ResolveThemeMetric(token);
    }
    return m_Context->GetThemeProvider().GetMetric(token);
}

Margin Widget::ThemePadding(ThemeToken token) const {
    if (!m_Context) {
        return ResolveThemePadding(token);
    }
    return m_Context->GetThemeProvider().GetPadding(token);
}

ResolvedStyle Widget::ResolveStyle(StyleRole role) const {
    return Styles().Resolve(role);
}

float Widget::Scaled(float logicalValue) const {
    return Styles().Scaled(logicalValue);
}

IThemeProvider& Widget::Theme() const {
    if (!m_Context) {
        return ResolveDefaultThemeProvider();
    }
    return m_Context->GetThemeProvider();
}

Color Widget::ThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    if (!m_Context) {
        return ResolveThemeInteractiveBackground(hoverAnim, pressAnim, selected);
    }
    return m_Context->GetThemeProvider().InteractiveBackground(hoverAnim, pressAnim, selected);
}

Color Widget::ThemeTextForState(bool hovered, bool active) const {
    if (!m_Context) {
        return ResolveThemeTextForState(hovered, active);
    }
    return m_Context->GetThemeProvider().TextForState(hovered, active);
}

Color Widget::ThemeIconForState(bool hovered, bool active) const {
    if (!m_Context) {
        return ResolveThemeIconForState(hovered, active);
    }
    return m_Context->GetThemeProvider().IconForState(hovered, active);
}

IPopupHost* Widget::GetPopupHost() const {
    return m_Context ? m_Context->GetPopupHost() : nullptr;
}

} // namespace WindEffects::Editor::UI

