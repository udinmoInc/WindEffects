#include "WindEffects/Runtime/UI/Input/InputEvents.h"

namespace WindEffects::Editor::UI {

char KeyCodeToChar(we::platform::KeyCode key, bool shift) {
    using KC = we::platform::KeyCode;
    if (key >= KC::A && key <= KC::Z) {
        const int i = static_cast<int>(key) - static_cast<int>(KC::A);
        return static_cast<char>((shift ? 'A' : 'a') + i);
    }
    if (key >= KC::Num0 && key <= KC::Num9) {
        return static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(KC::Num0)));
    }
    switch (key) {
    case KC::Space: return ' ';
    case KC::Minus: return shift ? '_' : '-';
    case KC::Equal: return shift ? '+' : '=';
    case KC::Period: return shift ? '>' : '.';
    case KC::Comma: return shift ? '<' : ',';
    case KC::Slash: return shift ? '?' : '/';
    case KC::Semicolon: return shift ? ':' : ';';
    case KC::Apostrophe: return shift ? '"' : '\'';
    case KC::LeftBracket: return shift ? '{' : '[';
    case KC::RightBracket: return shift ? '}' : ']';
    case KC::Backslash: return shift ? '|' : '\\';
    case KC::Grave: return shift ? '~' : '`';
    default: return '\0';
    }
}

} // namespace WindEffects::Editor::UI
