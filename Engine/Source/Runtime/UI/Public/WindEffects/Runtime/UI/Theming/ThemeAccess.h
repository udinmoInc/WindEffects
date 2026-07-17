#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Core/Types.h"

namespace WindEffects::Editor::UI {

UI_API IThemeProvider& ResolveDefaultThemeProvider();
UI_API Color ResolveThemeColor(ThemeToken token);
UI_API float ResolveThemeMetric(ThemeToken token);
UI_API Margin ResolveThemePadding(ThemeToken token);
UI_API Color ResolveThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected = false);
UI_API Color ResolveThemeTextForState(bool hovered, bool active = false);
UI_API Color ResolveThemeIconForState(bool hovered, bool active = false);

// Prefer ResolveIconColor / ResolveIconColorForState from ThemeColors.h for new code.

} // namespace WindEffects::Editor::UI
