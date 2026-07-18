#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

/// Concrete typography values resolved from a semantic role + theme.
/// Role → color mapping (IKindUITheme::ResolveTypography):
///   PageTitle / SectionTitle / Body → TextPrimary
///   Subtitle                        → TextSecondary
///   Caption / Label                 → TextCaption / TextSecondary
///   Hint / CaptionSmall             → TextHint
///   Disabled                        → TextDisabled
struct KINDUI_API TypographySpec {
    TypographyToken role = TypographyToken::Body;
    float sizePx = 13.0f;
    float lineHeightPx = 18.0f;
    float letterSpacing = 0.0f;
    /// Inter weight ladder: 400 Regular / 500 Medium / 600 SemiBold.
    uint16_t weight = 400;
    bool bold = false; // true when weight >= SemiBold (legacy DrawText flag)
    bool italic = false;
    Color color{};
};

} // namespace we::runtime::kindui
