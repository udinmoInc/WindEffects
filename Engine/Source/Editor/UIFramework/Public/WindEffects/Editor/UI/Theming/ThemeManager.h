#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Theming/IThemeProvider.h"
#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace WindEffects::Editor::UI {

// Single source of truth for the active theme + style resolver.
// Apps call Initialize once at startup; ThemeAccess and widgets read from here.
class UIFRAMEWORK_API ThemeManager {
public:
    using ChangeListener = std::function<void()>;

    static ThemeManager& Get();

    // Creates StyleResolver from provider. Safe to call multiple times (replaces theme).
    void Initialize(std::shared_ptr<IThemeProvider> theme, float dpiScale = 1.0f);

    // Swap theme pack at runtime (notifies listeners).
    void SetTheme(std::shared_ptr<IThemeProvider> theme);

    [[nodiscard]] bool IsInitialized() const { return m_Provider != nullptr; }
    [[nodiscard]] std::string_view GetThemeId() const;

    [[nodiscard]] IThemeProvider& Provider();
    [[nodiscard]] const IThemeProvider& Provider() const;
    [[nodiscard]] IStyleResolver& Styles();
    [[nodiscard]] const IStyleResolver& Styles() const;

    [[nodiscard]] std::shared_ptr<IThemeProvider> SharedProvider() const { return m_Provider; }
    [[nodiscard]] std::shared_ptr<IStyleResolver> SharedResolver() const { return m_Resolver; }

    void SetDpiScale(float scale);
    [[nodiscard]] float GetDpiScale() const;

    void AddChangeListener(ChangeListener listener);
    void ClearChangeListeners();

    // Convenience: resolve a role through the active resolver.
    [[nodiscard]] ResolvedStyle Resolve(StyleRole role) const;

private:
    ThemeManager() = default;
    void NotifyChanged();

    std::shared_ptr<IThemeProvider> m_Provider;
    std::shared_ptr<StyleResolver> m_Resolver;
    std::vector<ChangeListener> m_Listeners;
    mutable std::mutex m_Mutex;
};

} // namespace WindEffects::Editor::UI
