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
#include <optional>
#include <string>
#include <vector>

namespace we::runtime::kindui {

class Widget;
class EventSystem;

struct ControlDiagnosticResult {
    std::string name;
    std::string parentName;
    bool visible = false;
    bool effectivelyVisible = false;
    bool enabled = false;
    bool active = false;
    bool interactive = false;
    bool hitTestEnabled = false;
    bool focusable = false;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool mouseCaptured = false;
    bool disabledByParent = false;
    bool hasClickHandler = false;

    Rect geometry{};
    Rect hitTestGeometry{};

    std::vector<std::string> warnings;
    std::vector<std::string> parentChain;
    std::string rootFailureReason;
    std::string hitTestActualTarget;
    std::string blockingWidgetName;
    bool pass = true;
};

/// Runtime input/coordinate diagnostics for the click path root.
/// Enable with WE_UI_INPUT_DEBUG=1 (also enables verbose move/geometry logs).
class KINDUI_API UiInputDebug {
public:
    /// True when WE_UI_INPUT_DEBUG=1.
    static bool IsEnabled();
    /// True only when WE_UI_INPUT_DEBUG=1 (extra verbosity).
    static bool IsVerbose();

    static ControlDiagnosticResult ValidateInteractiveControl(
        const std::shared_ptr<Widget>& control,
        const std::shared_ptr<Widget>& rootWidget = nullptr);

    static void AuditControlTree(
        const std::shared_ptr<Widget>& rootWidget,
        const char* contextTag = "UI HEALTH");

    static void RunFullToolbarStatusBarDiagnostic(
        const std::shared_ptr<Widget>& rootWidget);

    static void TestFullInteractionPipeline(
        const std::shared_ptr<Widget>& rootWidget,
        EventSystem* eventSystem = nullptr);

    static void OnWidgetStateChange(
        const Widget& widget,
        const char* propertyName,
        bool oldValue,
        bool newValue,
        const char* callerInfo = nullptr);

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

    static std::string WidgetLabel(const std::shared_ptr<Widget>& widget);
};

} // namespace we::runtime::kindui

