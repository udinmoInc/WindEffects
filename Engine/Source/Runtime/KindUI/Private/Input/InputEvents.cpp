// ==============================================================================
// WindEffects — KindUI — InputEvents
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Input/InputEvents.h"

namespace we::runtime::kindui {

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

} // namespace we::runtime::kindui
 
