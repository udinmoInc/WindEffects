#include "WindEffects/Editor/UI/Theming/EditorTheme.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::ui {
namespace {

we::runtime::kindui::Color EdHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return we::runtime::kindui::Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

we::runtime::kindui::Color EditorTheme::ResolveColor(we::runtime::kindui::ColorToken token) const {
    switch (token) {
    case ColorToken::IconAccent:
    case ColorToken::AccentPrimary:
        return EdHex(0xF0, 0xA4, 0x2A);
    case ColorToken::AccentHover:
        return EdHex(0xF5, 0xB8, 0x45);
    case ColorToken::BorderFocus:
    case ColorToken::LinkForeground:
    case ColorToken::PlayForeground:
        return EdHex(0xF0, 0xA4, 0x2A);
    case ColorToken::ActiveTabLine:
        return EdHex(0xF0, 0xA4, 0x2A, 0.80f);
    case ColorToken::ButtonPrimaryBackground:
        return EdHex(0xC4, 0x7E, 0x18);
    case ColorToken::ButtonPrimaryHover:
        return EdHex(0xD9, 0x92, 0x28);
    case ColorToken::ButtonPrimaryPressed:
        return EdHex(0xA3, 0x68, 0x12);
    case ColorToken::SelectedBackground:
        return EdHex(0x3A, 0x32, 0x24);
    case ColorToken::SelectionHighlight:
        return EdHex(0xF0, 0xA4, 0x2A, 0.22f);
    default:
        return GraphiteDarkTheme::ResolveColor(token);
    }
}

} // namespace we::editor::ui
