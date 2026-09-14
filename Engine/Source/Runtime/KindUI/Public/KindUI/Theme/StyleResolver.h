// ==============================================================================
// WindEffects — KindUI — StyleResolver
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theme/IKindUITheme.h"
#include "KindUI/Theme/ResolvedStyle.h"

#include <array>
#include <memory>

namespace we::runtime::kindui {

/// Canonical StyleRole / style-class → ResolvedStyle resolver.
/// Owned by ThemeManager; widgets read through IStyleResolver / ThemeManager::Resolve.
class KINDUI_API StyleResolver final : public IStyleResolver {
public:
    explicit StyleResolver(std::shared_ptr<IKindUITheme> theme);

    [[nodiscard]] ResolvedStyle Resolve(StyleRole role) const override;
    [[nodiscard]] ResolvedStyle ResolveClass(std::string_view className) const override;
    [[nodiscard]] float Scaled(float logicalValue) const override;
    [[nodiscard]] float GetDpiScale() const override { return m_DpiScale; }
    void SetDpiScale(float scale) override;

private:
    std::shared_ptr<IKindUITheme> m_Theme;
    float m_DpiScale = 1.0f;
    mutable std::array<ResolvedStyle, 64> m_StyleCache{};
    mutable std::array<bool, 64> m_StyleCacheValid{};
    mutable uint64_t m_CachedThemeVersion{0};
    mutable float m_CachedDpiScale{0.0f};
};

} // namespace we::runtime::kindui
