#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/ThemeToken.h"
#include "KindUI/Theming/IThemeProvider.h"
#include "KindUI/Core/Types.h"

namespace we::runtime::kindui {

KINDUI_API IThemeProvider& ResolveDefaultThemeProvider();
KINDUI_API Color ResolveThemeColor(ThemeToken token);
KINDUI_API float ResolveThemeMetric(ThemeToken token);
KINDUI_API Margin ResolveThemePadding(ThemeToken token);
KINDUI_API Color ResolveThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected = false);
KINDUI_API Color ResolveThemeTextForState(bool hovered, bool active = false);
KINDUI_API Color ResolveThemeIconForState(bool hovered, bool active = false);

// Prefer ResolveIconColor / ResolveIconColorForState from ThemeColors.h for new code.

} // namespace we::runtime::kindui
