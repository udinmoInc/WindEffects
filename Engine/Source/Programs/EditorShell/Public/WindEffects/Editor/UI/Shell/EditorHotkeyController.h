// ==============================================================================
// WindEffects — EditorShell — EditorHotkeyController
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::shell {

#pragma warning(push)
#pragma warning(disable: 4251)

struct EDITORSHELL_API HotkeyBinding {
    std::string actionId;
    int keyCode = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    std::function<void()> action;
};

/// Central reusable hotkey dispatcher for editor panels, widgets, and viewports.
class EDITORSHELL_API EditorHotkeyController {
public:
    static EditorHotkeyController& Get();

    void RegisterHotkey(const std::string& context, HotkeyBinding binding);
    void UnregisterHotkey(const std::string& context, const std::string& actionId);
    void ClearContext(const std::string& context);

    bool ProcessKeyPress(const std::string& context, int keyCode, bool ctrl, bool shift, bool alt);

private:
    EditorHotkeyController() = default;

    std::unordered_map<std::string, std::vector<HotkeyBinding>> m_Bindings;
};

#pragma warning(pop)

} // namespace we::editor::shell
