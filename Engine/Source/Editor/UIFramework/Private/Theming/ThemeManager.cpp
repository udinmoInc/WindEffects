#include "WindEffects/Editor/UI/Theming/ThemeManager.h"

#include "Core/DPIContext.h"

#include <stdexcept>

namespace WindEffects::Editor::UI {

ThemeManager& ThemeManager::Get() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::Initialize(std::shared_ptr<IThemeProvider> theme, float dpiScale) {
    if (!theme) {
        throw std::invalid_argument("ThemeManager::Initialize requires a non-null theme");
    }
    {
        std::lock_guard lock(m_Mutex);
        m_Provider = std::move(theme);
        m_Resolver = std::make_shared<StyleResolver>(m_Provider);
        m_Resolver->SetDpiScale(dpiScale);
        DPIContext::SetScale(dpiScale);
    }
    NotifyChanged();
}

void ThemeManager::SetTheme(std::shared_ptr<IThemeProvider> theme) {
    if (!theme) {
        return;
    }
    float dpi = 1.0f;
    {
        std::lock_guard lock(m_Mutex);
        dpi = m_Resolver ? m_Resolver->GetDpiScale() : DPIContext::GetScale();
        m_Provider = std::move(theme);
        m_Resolver = std::make_shared<StyleResolver>(m_Provider);
        m_Resolver->SetDpiScale(dpi);
    }
    NotifyChanged();
}

std::string_view ThemeManager::GetThemeId() const {
    std::lock_guard lock(m_Mutex);
    if (!m_Provider) {
        return {};
    }
    return m_Provider->GetThemeId();
}

IThemeProvider& ThemeManager::Provider() {
    std::lock_guard lock(m_Mutex);
    if (!m_Provider) {
        // Lazy default so free ThemeAccess helpers work before app bootstrap.
        m_Provider = std::make_shared<GraphiteDarkTheme>();
        m_Resolver = std::make_shared<StyleResolver>(m_Provider);
        m_Resolver->SetDpiScale(std::max(1.0f, DPIContext::GetScale()));
    }
    return *m_Provider;
}

const IThemeProvider& ThemeManager::Provider() const {
    return const_cast<ThemeManager*>(this)->Provider();
}

IStyleResolver& ThemeManager::Styles() {
    std::lock_guard lock(m_Mutex);
    if (!m_Resolver) {
        if (!m_Provider) {
            m_Provider = std::make_shared<GraphiteDarkTheme>();
        }
        m_Resolver = std::make_shared<StyleResolver>(m_Provider);
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

} // namespace WindEffects::Editor::UI
