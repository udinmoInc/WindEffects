#include "WindEffects/Editor/UI/Theming/EditorTheme.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::services {

we::runtime::kindui::Color EditorTheme::ResolveColor(we::runtime::kindui::ColorToken token) const {
    // Editor uses the UE5-aligned GraphiteDark palette directly.
    return GraphiteDarkTheme::ResolveColor(token);
}

} // namespace we::editor::services
