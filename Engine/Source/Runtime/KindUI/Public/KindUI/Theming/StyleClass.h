// ==============================================================================
// WindEffects — KindUI — StyleClass
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

/// Declarative style class with optional inheritance (Button -> PrimaryButton).
struct KINDUI_API StyleClass {
    std::string name;
    std::string parentName;

    ColorToken background = ColorToken::PanelBackground;
    ColorToken foreground = ColorToken::TextPrimary;
    ColorToken border = ColorToken::BorderDefault;
    ColorToken hoverBackground = ColorToken::HoverBackground;
    ColorToken pressedBackground = ColorToken::PressedBackground;
    ColorToken disabledBackground = ColorToken::DisabledBackground;

    PaddingToken paddingToken = PaddingToken::Panel;
    MetricToken radiusToken = MetricToken::CornerRadiusMedium;
    MetricToken fontSizeToken = MetricToken::TextSizeBody;
    MetricToken heightToken = MetricToken::ButtonHeight;
    MetricToken animDurationToken = MetricToken::HoverAnimationDamping;

    float opacity = 1.0f;
    bool bold = false;
};

class KINDUI_API StyleClassRegistry {
public:
    static StyleClassRegistry& Get();

    void Register(StyleClass styleClass);
    void RegisterDefaults();
    [[nodiscard]] const StyleClass* Find(std::string_view name) const;
    [[nodiscard]] StyleClass Resolve(std::string_view name) const;

private:
    StyleClassRegistry() = default;
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, StyleClass> m_Classes;
};

} // namespace we::runtime::kindui
