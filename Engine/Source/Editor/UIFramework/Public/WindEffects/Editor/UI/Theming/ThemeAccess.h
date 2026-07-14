#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "WindEffects/Editor/UI/Theming/IThemeProvider.h"
#include "WindEffects/Editor/UI/Core/Types.h"

namespace WindEffects::Editor::UI {

UIFRAMEWORK_API IThemeProvider& ResolveDefaultThemeProvider();
UIFRAMEWORK_API Color ResolveThemeColor(ThemeToken token);
UIFRAMEWORK_API float ResolveThemeMetric(ThemeToken token);
UIFRAMEWORK_API Margin ResolveThemePadding(ThemeToken token);
UIFRAMEWORK_API Color ResolveThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected = false);
UIFRAMEWORK_API Color ResolveThemeTextForState(bool hovered, bool active = false);
UIFRAMEWORK_API Color ResolveThemeIconForState(bool hovered, bool active = false);

// Prefer ResolveIconColor / ResolveIconColorForState from ThemeColors.h for new code.

} // namespace WindEffects::Editor::UI
