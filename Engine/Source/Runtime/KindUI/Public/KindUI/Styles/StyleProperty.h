#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Tokens/DesignToken.h"

#include <optional>

namespace we::runtime::kindui {

// A style property references a design token with an optional local override.
template<typename TokenEnum, typename ValueType>
struct StyleProperty {
    TokenEnum token{};
    std::optional<ValueType> override{};

    StyleProperty() = default;
    explicit StyleProperty(TokenEnum t) : token(t) {}
    StyleProperty(TokenEnum t, ValueType local) : token(t), override(local) {}

    [[nodiscard]] bool HasOverride() const { return override.has_value(); }
};

using ColorProperty = StyleProperty<ColorToken, Color>;
using SpacingProperty = StyleProperty<SpacingToken, float>;
using RadiusProperty = StyleProperty<RadiusToken, float>;
using TypographyProperty = StyleProperty<TypographyToken, float>;
using ElevationProperty = StyleProperty<ElevationToken, int>;
using AnimationProperty = StyleProperty<AnimationToken, float>;

struct ShadowStyleTokens {
    ColorProperty color{ColorToken::ShadowColor};
    SpacingProperty blur{SpacingToken::Small};
    SpacingProperty spread{SpacingToken::None};
    SpacingProperty offsetX{SpacingToken::None};
    SpacingProperty offsetY{SpacingToken::Small};
};

struct BorderStyleTokens {
    ColorProperty color{ColorToken::BorderDefault};
    SpacingProperty width{SpacingToken::None};
    RadiusProperty radius{RadiusToken::Small};
};

struct PaddingStyle {
    SpacingProperty left{SpacingToken::Medium};
    SpacingProperty top{SpacingToken::Medium};
    SpacingProperty right{SpacingToken::Medium};
    SpacingProperty bottom{SpacingToken::Medium};
};

struct TypographyStyle {
    TypographyProperty token{TypographyToken::Body};
    ColorProperty color{ColorToken::TextPrimary};
    std::optional<bool> bold{};

    TypographyStyle() = default;
    explicit TypographyStyle(TypographyToken t) : token(t) {}
};

} // namespace we::runtime::kindui
