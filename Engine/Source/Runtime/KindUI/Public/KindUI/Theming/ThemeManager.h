#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/GraphiteDarkTheme.h"
#include "KindUI/Theming/ResolvedStyle.h"
#include "KindUI/Theming/StyleRole.h"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace we::runtime::kindui {

// Single source of truth for the active theme + style resolver.
// Apps call Initialize once at startup; ThemeAccess and widgets read from here.
class KINDUI_API ThemeManager {
public:
    using ChangeListener = std::function<void()>;

    static ThemeManager& Get();

    // Creates StyleResolver from theme. Safe to call multiple times (replaces theme).
    void Initialize(std::shared_ptr<IKindUITheme> theme, float dpiScale = 1.0f);

    // Swap theme pack at runtime (notifies listeners).
    void SetTheme(std::shared_ptr<IKindUITheme> theme);

    [[nodiscard]] bool IsInitialized() const { return m_Theme != nullptr; }
    [[nodiscard]] std::string_view GetThemeId() const;

    [[nodiscard]] IKindUITheme& Theme();
    [[nodiscard]] const IKindUITheme& Theme() const;
    [[nodiscard]] IStyleResolver& Styles();
    [[nodiscard]] const IStyleResolver& Styles() const;

    [[nodiscard]] std::shared_ptr<IKindUITheme> SharedTheme() const { return m_Theme; }
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

    std::shared_ptr<IKindUITheme> m_Theme;
    std::shared_ptr<StyleResolver> m_Resolver;
    std::vector<ChangeListener> m_Listeners;
    mutable std::mutex m_Mutex;
};

} // namespace we::runtime::kindui
