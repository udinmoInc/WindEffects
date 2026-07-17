#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/StyleClass.h"
#include "KindUI/Core/DPIContext.h"

#include <algorithm>
#include <stdexcept>

namespace we::runtime::kindui {

ThemeManager& ThemeManager::Get() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::Initialize(std::shared_ptr<IKindUITheme> theme, float dpiScale) {
    if (!theme) {
        throw std::invalid_argument("ThemeManager::Initialize requires a non-null theme");
    }
    {
        std::lock_guard lock(m_Mutex);
        m_Theme = std::move(theme);
        m_Resolver = std::make_shared<StyleResolver>(m_Theme);
        m_Resolver->SetDpiScale(dpiScale);
        DPIContext::SetScale(dpiScale);
    }
    StyleClassRegistry::Get().RegisterDefaults();
    NotifyChanged();
}

void ThemeManager::SetTheme(std::shared_ptr<IKindUITheme> theme) {
    if (!theme) {
        return;
    }
    float dpi = 1.0f;
    {
        std::lock_guard lock(m_Mutex);
        dpi = m_Resolver ? m_Resolver->GetDpiScale() : DPIContext::GetScale();
        m_Theme = std::move(theme);
        m_Resolver = std::make_shared<StyleResolver>(m_Theme);
        m_Resolver->SetDpiScale(dpi);
    }
    NotifyChanged();
}

std::string_view ThemeManager::GetThemeId() const {
    std::lock_guard lock(m_Mutex);
    if (!m_Theme) {
        return {};
    }
    return m_Theme->GetThemeId();
}

IKindUITheme& ThemeManager::Theme() {
    std::lock_guard lock(m_Mutex);
    if (!m_Theme) {
        m_Theme = std::make_shared<GraphiteDarkTheme>();
        m_Resolver = std::make_shared<StyleResolver>(m_Theme);
        m_Resolver->SetDpiScale(std::max(1.0f, DPIContext::GetScale()));
    }
    return *m_Theme;
}

const IKindUITheme& ThemeManager::Theme() const {
    return const_cast<ThemeManager*>(this)->Theme();
}

IStyleResolver& ThemeManager::Styles() {
    std::lock_guard lock(m_Mutex);
    if (!m_Resolver) {
        if (!m_Theme) {
            m_Theme = std::make_shared<GraphiteDarkTheme>();
        }
        m_Resolver = std::make_shared<StyleResolver>(m_Theme);
        m_Resolver->SetDpiScale(std::max(1.0f, DPIContext::GetScale()));
    }
    return *m_Resolver;
}

const IStyleResolver& ThemeManager::Styles() const {
    return const_cast<ThemeManager*>(this)->Styles();
}

void ThemeManager::SetDpiScale(float scale) {
    std::lock_guard lock(m_Mutex);
    if (m_Resolver) {
        m_Resolver->SetDpiScale(scale);
    }
    DPIContext::SetScale(scale);
}

float ThemeManager::GetDpiScale() const {
    std::lock_guard lock(m_Mutex);
    return m_Resolver ? m_Resolver->GetDpiScale() : DPIContext::GetScale();
}

void ThemeManager::AddChangeListener(ChangeListener listener) {
    if (!listener) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    m_Listeners.push_back(std::move(listener));
}

void ThemeManager::ClearChangeListeners() {
    std::lock_guard lock(m_Mutex);
    m_Listeners.clear();
}

ResolvedStyle ThemeManager::Resolve(StyleRole role) const {
    return Styles().Resolve(role);
}

void ThemeManager::NotifyChanged() {
    std::vector<ChangeListener> copy;
    {
        std::lock_guard lock(m_Mutex);
        copy = m_Listeners;
    }
    for (auto& listener : copy) {
        if (listener) {
            listener();
        }
    }
}

} // namespace we::runtime::kindui
