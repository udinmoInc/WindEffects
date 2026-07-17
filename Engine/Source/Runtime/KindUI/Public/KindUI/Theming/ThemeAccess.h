#pragma once

#include "KindUI/Export.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Core/Types.h"

namespace we::runtime::kindui {

KINDUI_API IKindUITheme& ResolveDefaultTheme();
KINDUI_API Color ResolveColor(ColorToken token);
KINDUI_API float ResolveMetric(MetricToken token);
KINDUI_API Margin ResolvePadding(PaddingToken token);
KINDUI_API float ResolveSpacing(SpacingToken token);
KINDUI_API float ResolveRadius(RadiusToken token);
KINDUI_API float ResolveFontSize(TypographyToken token);
KINDUI_API TypographySpec ResolveTypography(TypographyToken token);
KINDUI_API Color ResolveInteractiveBackground(float hoverAnim, float pressAnim, bool selected = false);
KINDUI_API Color ResolveTextForState(bool hovered, bool active = false);
KINDUI_API Color ResolveIconForState(bool hovered, bool active = false);

} // namespace we::runtime::kindui
