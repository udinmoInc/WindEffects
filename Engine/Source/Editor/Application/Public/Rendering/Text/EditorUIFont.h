#pragma once

#include "Application/Export.h"

#include <string>

namespace we::UI::Text {

struct APPLICATION_API EditorUIFontPaths {
    std::string regular;
    std::string semibold;
    std::string fallback;
    std::string familyName;
};

// Resolves professional UI fonts (Segoe UI Variable on Windows, Inter fallback).
APPLICATION_API EditorUIFontPaths ResolveEditorUIFonts();

} // namespace we::UI::Text
