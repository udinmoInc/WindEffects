#include "WindEffects/Editor/UI/Theming/EditorTheme.h"

namespace we::editor::ui {
namespace {

we::runtime::kindui::Color EdHex(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return we::runtime::kindui::Color{ r / 255.0f, g / 255.0f, b / 255.0f, a };
}

} // namespace

we::runtime::kindui::Color EditorTheme::GetColor(we::runtime::kindui::ThemeToken token) const {
    using we::runtime::kindui::ThemeToken;
    switch (token) {
    case ThemeToken::IconAccent:
    case ThemeToken::AccentPrimary:
        return EdHex(0xF0, 0xA4, 0x2A);
    case ThemeToken::AccentHover:
        return EdHex(0xF5, 0xB8, 0x45);
    case ThemeToken::BorderFocus:
    case ThemeToken::LinkForeground:
    case ThemeToken::PlayForeground:
        return EdHex(0xF0, 0xA4, 0x2A);
    case ThemeToken::ActiveTabLine:
        return EdHex(0xF0, 0xA4, 0x2A, 0.80f);
    case ThemeToken::ButtonPrimaryBackground:
        return EdHex(0xC4, 0x7E, 0x18);
    case ThemeToken::ButtonPrimaryHover:
        return EdHex(0xD9, 0x92, 0x28);
    case ThemeToken::ButtonPrimaryPressed:
        return EdHex(0xA3, 0x68, 0x12);
    case ThemeToken::SelectedBackground:
        return EdHex(0x3A, 0x32, 0x24);
    case ThemeToken::SelectionHighlight:
        return EdHex(0xF0, 0xA4, 0x2A, 0.22f);
    default:
        return GraphiteDarkTheme::GetColor(token);
    }
}

} // namespace we::editor::ui
