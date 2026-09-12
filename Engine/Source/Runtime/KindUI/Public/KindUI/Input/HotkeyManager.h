// ==============================================================================
// WindEffects — KindUI — HotkeyManager
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Input/InputEvents.h"
#include "Platform/InputTypes.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

#pragma warning(push)
#pragma warning(disable: 4251)

/// Represents a physical key and modifier combination.
struct KINDUI_API HotkeyChord {
    we::platform::KeyCode key = we::platform::KeyCode::Unknown;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    bool operator==(const HotkeyChord& other) const {
        return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt;
    }
    bool operator!=(const HotkeyChord& other) const { return !(*this == other); }
    bool IsValid() const { return key != we::platform::KeyCode::Unknown; }

    std::string ToString() const;
    static HotkeyChord FromString(const std::string& str);
    static HotkeyChord FromKeyEvent(const KeyEvent& event);
};

/// Represents a configurable action binding.
struct KINDUI_API HotkeyBinding {
    std::string actionId;        // e.g. "Editor.SelectAll"
    std::string displayName;     // e.g. "Select All"
    std::string category;        // e.g. "Editor", "Viewport", "Global"
    std::string description;     // e.g. "Selects all items in current view"
    HotkeyChord chord;           // Active key chord
    HotkeyChord defaultChord;    // Default key chord
    std::function<bool()> action; // Callback (returns true if handled)
    int priority = 0;            // Higher priority evaluated first
    bool enabled = true;
};

/// Engine-wide customizable Hotkey Manager and Dispatcher.
class KINDUI_API HotkeyManager {
public:
    static HotkeyManager& Get();

    /// Register a named hotkey action with callback and metadata.
    void RegisterAction(const std::string& actionId,
                        const std::string& displayName,
                        const std::string& category,
                        const HotkeyChord& defaultChord,
                        std::function<bool()> action,
                        int priority = 0,
                        const std::string& description = "");

    /// Unregister an existing action.
    void UnregisterAction(const std::string& actionId);

    /// Rebind an action's shortcut dynamically at runtime.
    bool RebindAction(const std::string& actionId, const HotkeyChord& newChord);

    /// Reset a specific action or all actions to default bindings.
    bool ResetActionToDefault(const std::string& actionId);
    void ResetAllToDefaults();

    /// Enable or disable a registered action.
    void SetActionEnabled(const std::string& actionId, bool enabled);

    /// Retrieve binding metadata.
    bool GetBinding(const std::string& actionId, HotkeyBinding& outBinding) const;
    std::vector<HotkeyBinding> GetAllBindings() const;
    std::vector<HotkeyBinding> GetBindingsByCategory(const std::string& category) const;

    /// Check if a chord conflicts with existing registered actions in a category.
    bool CheckConflict(const HotkeyChord& chord, const std::string& category, std::string& outConflictingActionId)
        const;

    /// Dispatch a key event to matching registered action callbacks.
    bool Dispatch(const KeyEvent& event, const std::string& activeCategory = "");

    /// Pre-registers standard editor hotkeys across Global, Editor, Viewport, ContentBrowser, and Console categories.
    void RegisterStandardEditorHotkeys();

    /// Load and save user customized key bindings from/to JSON config files.
    bool LoadCustomBindings(const std::string& filePath);
    bool SaveCustomBindings(const std::string& filePath) const;
    std::string ExportBindingsJson() const;
    bool ImportBindingsJson(const std::string& jsonContent);

private:
    HotkeyManager();
    ~HotkeyManager() = default;

    std::unordered_map<std::string, HotkeyBinding> m_Bindings;
};

#pragma warning(pop)

} // namespace we::runtime::kindui
