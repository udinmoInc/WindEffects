// ==============================================================================
// WindEffects — EditorShell — EditorHotkeyController
// Internal implementation for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Shell/EditorHotkeyController.h"
#include "KindUI/Input/HotkeyManager.h"
#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"

#include <algorithm>

namespace we::editor::shell {

EditorHotkeyController& EditorHotkeyController::Get() {
    static EditorHotkeyController instance;
    return instance;
}

void EditorHotkeyController::RegisterHotkey(const std::string& context, HotkeyBinding binding) {
    we::runtime::kindui::HotkeyChord chord;
    chord.key = static_cast<we::platform::KeyCode>(binding.keyCode);
    chord.ctrl = binding.ctrl;
    chord.shift = binding.shift;
    chord.alt = binding.alt;

    auto actionWrapper = [fn = std::move(binding.action)]() -> bool {
        if (fn) {
            fn();
            return true;
        }
        return false;
    };

    we::runtime::kindui::HotkeyManager::Get().RegisterAction(
        binding.actionId, binding.actionId, context, chord, actionWrapper, 100);

    auto& vec = m_Bindings[context];
    auto it = std::find_if(vec.begin(), vec.end(), [&](const HotkeyBinding& b) {
        return b.actionId == binding.actionId;
    });
    if (it != vec.end()) {
        *it = std::move(binding);
    } else {
        vec.push_back(std::move(binding));
    }
}

void EditorHotkeyController::UnregisterHotkey(const std::string& context, const std::string& actionId) {
    we::runtime::kindui::HotkeyManager::Get().UnregisterAction(actionId);

    auto it = m_Bindings.find(context);
    if (it != m_Bindings.end()) {
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const HotkeyBinding& b) {
            return b.actionId == actionId;
        }), vec.end());
    }
}

void EditorHotkeyController::ClearContext(const std::string& context) {
    for (const auto& binding : m_Bindings[context]) {
        we::runtime::kindui::HotkeyManager::Get().UnregisterAction(binding.actionId);
    }
    m_Bindings.erase(context);
}

bool EditorHotkeyController::ProcessKeyPress(const std::string& context, int keyCode, bool ctrl, bool shift, bool alt) {
    we::runtime::kindui::KeyEvent event;
    event.type = we::runtime::kindui::KeyEventType::KeyDown;
    event.key = static_cast<we::platform::KeyCode>(keyCode);
    event.ctrlDown = ctrl;
    event.shiftDown = shift;
    event.altDown = alt;

    if (we::runtime::kindui::HotkeyManager::Get().Dispatch(event, context)) {
        return true;
    }

    auto it = m_Bindings.find(context);
    if (it == m_Bindings.end()) {
        return false;
    }

    for (const auto& binding : it->second) {
        if (binding.keyCode == keyCode &&
            binding.ctrl == ctrl &&
            binding.shift == shift &&
            binding.alt == alt) {
            if (binding.action) {
                WE_LOG_DEBUG(we::LogCategory::General.data(),
                    "[Hotkey] Triggered action '" + binding.actionId + "' in context '" + context + "'");
                binding.action();
                return true;
            }
        }
    }
    return false;
}

} // namespace we::editor::shell
