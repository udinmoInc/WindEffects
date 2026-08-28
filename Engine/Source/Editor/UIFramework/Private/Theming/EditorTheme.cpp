#include "WindEffects/Editor/UI/Theming/EditorTheme.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::services {
namespace {

we::runtime::kindui::Color EdHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return we::runtime::kindui::Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

we::runtime::kindui::Color EditorTheme::ResolveColor(we::runtime::kindui::ColorToken token) const {
    using ::we::runtime::kindui::ColorToken;
    switch (token) {
    // Neutral Grayscale Accent (replacing Sleek Blue)
    case ColorToken::IconAccent:
        return EdHex(0xE6, 0xE6, 0xE6);
    case ColorToken::AccentPrimary:
    case ColorToken::BorderFocus:
    case ColorToken::LinkForeground:
    case ColorToken::PlayForeground:
        return EdHex(0x38, 0x39, 0x3C); 
    case ColorToken::AccentHover:
        return EdHex(0x40, 0x44, 0x4A);
    case ColorToken::ActiveTabLine:
        return EdHex(0x38, 0x39, 0x3C, 0.80f);
        
    // Primary Buttons (Neutral Charcoal)
    case ColorToken::ButtonPrimaryBackground:
        return EdHex(0x1D, 0x1E, 0x20);
    case ColorToken::ButtonPrimaryHover:
        return EdHex(0x27, 0x28, 0x2B);
    case ColorToken::ButtonPrimaryPressed:
        return EdHex(0x14, 0x15, 0x17);
        
    // Selection Highlights (Neutral Charcoal)
    case ColorToken::SelectedBackground:
        return EdHex(0x32, 0x33, 0x36);
    case ColorToken::SelectionHighlight:
        return EdHex(0x32, 0x33, 0x36, 0.90f);
    default:
        return GraphiteDarkTheme::ResolveColor(token);
    }
}

} // namespace we::editor::services
