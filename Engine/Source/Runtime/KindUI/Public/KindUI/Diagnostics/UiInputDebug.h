// ==============================================================================
// WindEffects — KindUI — UiInputDebug
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Core/InputEvents.h"

#include <memory>
#include <string>

namespace we::runtime::kindui {

class Widget;

/// Runtime input/coordinate diagnostics for the click path root.
/// Enable with WE_UI_INPUT_DEBUG=1 (also enables verbose move/geometry logs).
class KINDUI_API UiInputDebug {
public:
    /// True when WE_UI_INPUT_DEBUG=1.
    static bool IsEnabled();
    /// True only when WE_UI_INPUT_DEBUG=1 (extra verbosity).
    static bool IsVerbose();

    static void OnMouseEvent(
        const MouseEvent& event,
        const Point& rootPos,
        const std::shared_ptr<Widget>& hitWidget,
        const std::shared_ptr<Widget>& targetWidget,
        const std::shared_ptr<Widget>& focusedWidget,
        const std::shared_ptr<Widget>& capturedWidget);

    static void OnTextInput(char32_t codepoint, const std::shared_ptr<Widget>& focusedWidget);

    /// Root-of-execution log when a ToolButton (or similar) invokes its click callback.
    static void OnClickInvoked(const char* source, const std::string& label);

    static void OnHoverChanged(
        const std::shared_ptr<Widget>& oldWidget,
        const std::shared_ptr<Widget>& newWidget,
        const Point& pos);

    static void LogWidgetGeometry(const char* label, const std::shared_ptr<Widget>& widget);

private:
    static std::string WidgetLabel(const std::shared_ptr<Widget>& widget);
};

} // namespace we::runtime::kindui
