// ==============================================================================
// WindEffects — KindUI — HotkeyManager
// Private implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Input/HotkeyManager.h"
#include "Core/Logger.h"
#include "Core/LogCategory.h"
#include "Core/DiagnosticMacros.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace we::runtime::kindui {

namespace {

std::string KeyCodeToString(we::platform::KeyCode key) {
    using we::platform::KeyCode;
    switch (key) {
        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";

        case KeyCode::Num0: return "0";
        case KeyCode::Num1: return "1";
        case KeyCode::Num2: return "2";
        case KeyCode::Num3: return "3";
        case KeyCode::Num4: return "4";
        case KeyCode::Num5: return "5";
        case KeyCode::Num6: return "6";
        case KeyCode::Num7: return "7";
        case KeyCode::Num8: return "8";
        case KeyCode::Num9: return "9";

        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";

        case KeyCode::Escape: return "Escape";
        case KeyCode::Tab: return "Tab";
        case KeyCode::CapsLock: return "CapsLock";
        case KeyCode::Space: return "Space";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Insert: return "Insert";
        case KeyCode::Delete: return "Delete";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PageUp";
        case KeyCode::PageDown: return "PageDown";

        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";
        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";

        case KeyCode::Grave: return "~";
        case KeyCode::Minus: return "-";
        case KeyCode::Equal: return "=";
        case KeyCode::LeftBracket: return "[";
        case KeyCode::RightBracket: return "]";
        case KeyCode::Backslash: return "\\";
        case KeyCode::Semicolon: return ";";
        case KeyCode::Apostrophe: return "'";
        case KeyCode::Comma: return ",";
        case KeyCode::Period: return ".";
        case KeyCode::Slash: return "/";

        default: return "Unknown";
    }
}

we::platform::KeyCode StringToKeyCode(const std::string& str) {
    using we::platform::KeyCode;
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c)); });

    if (upperStr.size() == 1) {
        char c = upperStr[0];
        if (c >= 'A' && c <= 'Z') return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (c - 'A'));
        if (c >= '0' && c <= '9') return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (c - '0'));
        if (c == '~' || c == '`') return KeyCode::Grave;
        if (c == '-') return KeyCode::Minus;
        if (c == '=') return KeyCode::Equal;
        if (c == '[') return KeyCode::LeftBracket;
        if (c == ']') return KeyCode::RightBracket;
        if (c == '\\') return KeyCode::Backslash;
        if (c == ';') return KeyCode::Semicolon;
        if (c == '\'') return KeyCode::Apostrophe;
        if (c == ',') return KeyCode::Comma;
        if (c == '.') return KeyCode::Period;
        if (c == '/') return KeyCode::Slash;
    }

    if (upperStr == "ESCAPE" || upperStr == "ESC") return KeyCode::Escape;
    if (upperStr == "TAB") return KeyCode::Tab;
    if (upperStr == "CAPSLOCK") return KeyCode::CapsLock;
    if (upperStr == "SPACE") return KeyCode::Space;
    if (upperStr == "ENTER" || upperStr == "RETURN") return KeyCode::Enter;
    if (upperStr == "BACKSPACE") return KeyCode::Backspace;
    if (upperStr == "INSERT" || upperStr == "INS") return KeyCode::Insert;
    if (upperStr == "DELETE" || upperStr == "DEL") return KeyCode::Delete;
    if (upperStr == "HOME") return KeyCode::Home;
    if (upperStr == "END") return KeyCode::End;
    if (upperStr == "PAGEUP" || upperStr == "PGUP") return KeyCode::PageUp;
    if (upperStr == "PAGEDOWN" || upperStr == "PGDN") return KeyCode::PageDown;

    if (upperStr == "LEFT") return KeyCode::Left;
    if (upperStr == "RIGHT") return KeyCode::Right;
    if (upperStr == "UP") return KeyCode::Up;
    if (upperStr == "DOWN") return KeyCode::Down;

    if (upperStr == "F1") return KeyCode::F1;
    if (upperStr == "F2") return KeyCode::F2;
    if (upperStr == "F3") return KeyCode::F3;
    if (upperStr == "F4") return KeyCode::F4;
    if (upperStr == "F5") return KeyCode::F5;
    if (upperStr == "F6") return KeyCode::F6;
    if (upperStr == "F7") return KeyCode::F7;
    if (upperStr == "F8") return KeyCode::F8;
    if (upperStr == "F9") return KeyCode::F9;
    if (upperStr == "F10") return KeyCode::F10;
    if (upperStr == "F11") return KeyCode::F11;
    if (upperStr == "F12") return KeyCode::F12;

    return KeyCode::Unknown;
}

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

} // namespace

std::string HotkeyChord::ToString() const {
    if (!IsValid()) return "None";
    std::string result;
    if (ctrl) result += "Ctrl+";
    if (shift) result += "Shift+";
    if (alt) result += "Alt+";
    result += KeyCodeToString(key);
    return result;
}

HotkeyChord HotkeyChord::FromString(const std::string& str) {
    HotkeyChord chord;
    std::string trimmed = Trim(str);
    if (trimmed.empty() || trimmed == "None") return chord;

    std::stringstream ss(trimmed);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, '+')) {
        tokens.push_back(Trim(token));
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string upperToken = tokens[i];
        std::transform(upperToken.begin(), upperToken.end(), upperToken.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c)); });

        if (upperToken == "CTRL" || upperToken == "CONTROL") {
            chord.ctrl = true;
        } else if (upperToken == "SHIFT") {
            chord.shift = true;
        } else if (upperToken == "ALT") {
            chord.alt = true;
        } else {
            chord.key = StringToKeyCode(tokens[i]);
        }
    }
    return chord;
}

HotkeyChord HotkeyChord::FromKeyEvent(const KeyEvent& event) {
    HotkeyChord chord;
    chord.key = event.key;
    chord.ctrl = event.ctrlDown;
    chord.shift = event.shiftDown;
    chord.alt = event.altDown;
    return chord;
}

HotkeyManager& HotkeyManager::Get() {
    static HotkeyManager instance;
    return instance;
}

HotkeyManager::HotkeyManager() {
    RegisterStandardEditorHotkeys();
}

void HotkeyManager::RegisterAction(
    const std::string& actionId,
    const std::string& displayName,
    const std::string& category,
    const HotkeyChord& defaultChord,
    std::function<bool()> action,
    int priority,
    const std::string& description)
{
    HotkeyBinding binding;
    binding.actionId = actionId;
    binding.displayName = displayName;
    binding.category = category;
    binding.description = description;
    binding.chord = defaultChord;
    binding.defaultChord = defaultChord;
    binding.action = action;
    binding.priority = priority;
    binding.enabled = true;

    auto it = m_Bindings.find(actionId);
    if (it != m_Bindings.end()) {
        binding.chord = it->second.chord;
    }

    m_Bindings[actionId] = std::move(binding);
}

void HotkeyManager::UnregisterAction(const std::string& actionId) {
    m_Bindings.erase(actionId);
}

bool HotkeyManager::RebindAction(const std::string& actionId, const HotkeyChord& newChord) {
    auto it = m_Bindings.find(actionId);
    if (it == m_Bindings.end()) {
        return false;
    }
    it->second.chord = newChord;
    WE_LOG_INFO(we::LogCategory::General.data(),
        "[HotkeyManager] Rebound action '" + actionId + "' to '" + newChord.ToString() + "'");
    return true;
}

bool HotkeyManager::ResetActionToDefault(const std::string& actionId) {
    auto it = m_Bindings.find(actionId);
    if (it == m_Bindings.end()) {
        return false;
    }
    it->second.chord = it->second.defaultChord;
    return true;
}

void HotkeyManager::ResetAllToDefaults() {
    for (auto& [id, binding] : m_Bindings) {
        binding.chord = binding.defaultChord;
    }
    WE_LOG_INFO(we::LogCategory::General.data(), "[HotkeyManager] Reset all hotkeys to default bindings");
}

void HotkeyManager::SetActionEnabled(const std::string& actionId, bool enabled) {
    auto it = m_Bindings.find(actionId);
    if (it != m_Bindings.end()) {
        it->second.enabled = enabled;
    }
}

bool HotkeyManager::GetBinding(const std::string& actionId, HotkeyBinding& outBinding) const {
    auto it = m_Bindings.find(actionId);
    if (it != m_Bindings.end()) {
        outBinding = it->second;
        return true;
    }
    return false;
}

std::vector<HotkeyBinding> HotkeyManager::GetAllBindings() const {
    std::vector<HotkeyBinding> list;
    list.reserve(m_Bindings.size());
    for (const auto& [id, binding] : m_Bindings) {
        list.push_back(binding);
    }
    return list;
}

std::vector<HotkeyBinding> HotkeyManager::GetBindingsByCategory(const std::string& category) const {
    std::vector<HotkeyBinding> list;
    for (const auto& [id, binding] : m_Bindings) {
        if (binding.category == category) {
            list.push_back(binding);
        }
    }
    return list;
}

bool HotkeyManager::CheckConflict(const HotkeyChord& chord, const std::string& category, std::string&
    outConflictingActionId) const {
    if (!chord.IsValid()) return false;
    for (const auto& [id, binding] : m_Bindings) {
        if (binding.chord == chord && (binding.category == category || binding.category == "Global" ||
            category.empty())) {
            outConflictingActionId = id;
            return true;
        }
    }
    return false;
}

bool HotkeyManager::Dispatch(const KeyEvent& event, const std::string& activeCategory) {
    if (event.type != KeyEventType::KeyDown) {
        return false;
    }

    const HotkeyChord eventChord = HotkeyChord::FromKeyEvent(event);
    if (!eventChord.IsValid()) {
        return false;
    }

    std::vector<const HotkeyBinding*> candidates;
    for (const auto& [id, binding] : m_Bindings) {
        if (binding.enabled && binding.chord == eventChord && binding.action) {
            candidates.push_back(&binding);
        }
    }

    if (candidates.empty()) {
        return false;
    }

    std::sort(candidates.begin(), candidates.end(), [&](const HotkeyBinding* a, const HotkeyBinding* b) {
        const bool aActiveCategory = (a->category == activeCategory);
        const bool bActiveCategory = (b->category == activeCategory);
        if (aActiveCategory != bActiveCategory) return aActiveCategory;

        const bool aGlobal = (a->category == "Global");
        const bool bGlobal = (b->category == "Global");
        if (aGlobal != bGlobal) return !aGlobal;

        return a->priority > b->priority;
    });

    for (const auto* binding : candidates) {
        if (binding->action && binding->action()) {
            WE_LOG_INFO(we::LogCategory::General.data(),
                "[HotkeyManager] Dispatched '" + binding->actionId + "' (" + binding->displayName + ") for chord '" +
                    eventChord.ToString() + "'");
            return true;
        }
    }

    return false;
}

void HotkeyManager::RegisterStandardEditorHotkeys() {
    using we::platform::KeyCode;

    // --- Global Category ---
    RegisterAction("Global.Save", "Save Project", "Global", {KeyCode::S, true, false, false}, nullptr, 100,
        "Saves active project/assets");
    RegisterAction("Global.SaveAll", "Save All", "Global", {KeyCode::S, true, true, false}, nullptr, 100,
        "Saves all open project files");
    RegisterAction("Global.Undo", "Undo", "Global", {KeyCode::Z, true, false, false}, nullptr, 100,
        "Undoes last action");
    RegisterAction("Global.Redo", "Redo", "Global", {KeyCode::Y, true, false, false}, nullptr, 100,
        "Redoes last action");
    RegisterAction("Global.Cut", "Cut", "Global", {KeyCode::X, true, false, false}, nullptr, 90,
        "Cuts current selection");
    RegisterAction("Global.Copy", "Copy", "Global", {KeyCode::C, true, false, false}, nullptr, 90,
        "Copies current selection");
    RegisterAction("Global.Paste", "Paste", "Global", {KeyCode::V, true, false, false}, nullptr, 90,
        "Pastes clipboard content");
    RegisterAction("Global.Search", "Search", "Global", {KeyCode::F, true, false, false}, nullptr, 80,
        "Opens global search dialog");
    RegisterAction("Global.CommandPalette", "Command Palette", "Global", {KeyCode::P, true, true, false}, nullptr, 110,
        "Opens editor command palette");

    // --- Editor / Selection Category ---
    RegisterAction("Editor.SelectAll", "Select All", "Editor", {KeyCode::A, true, false, false}, nullptr, 80,
        "Selects all visible items");
    RegisterAction("Editor.DeselectAll", "Deselect All", "Editor", {KeyCode::Escape, false, false, false}, nullptr, 80,
        "Clears current selection");
    RegisterAction("Editor.Duplicate", "Duplicate", "Editor", {KeyCode::D, true, false, false}, nullptr, 80,
        "Duplicates selected object");
    RegisterAction("Editor.Delete", "Delete", "Editor", {KeyCode::Delete, false, false, false}, nullptr, 80,
        "Deletes selected object");
    RegisterAction("Editor.Rename", "Rename", "Editor", {KeyCode::F2, false, false, false}, nullptr, 80,
        "Renames selected item");
    RegisterAction("Editor.Group", "Group Selection", "Editor", {KeyCode::G, true, false, false}, nullptr, 70,
        "Groups selected items");
    RegisterAction("Editor.Ungroup", "Ungroup Selection", "Editor", {KeyCode::G, false, true, false}, nullptr, 70,
        "Ungroups selected items");

    // --- Viewport Transformation & Camera Category ---
    RegisterAction("Viewport.TranslateMode", "Translate Tool", "Viewport", {KeyCode::W, false, false, false}, nullptr,
        70, "Selects translation transform gizmo");
    RegisterAction("Viewport.RotateMode", "Rotate Tool", "Viewport", {KeyCode::E, false, false, false}, nullptr, 70,
        "Selects rotation transform gizmo");
    RegisterAction("Viewport.ScaleMode", "Scale Tool", "Viewport", {KeyCode::R, false, false, false}, nullptr, 70,
        "Selects scale transform gizmo");
    RegisterAction("Viewport.Focus", "Focus Selection", "Viewport", {KeyCode::F, false, false, false}, nullptr, 70,
        "Focuses camera on selected object");
    RegisterAction("Viewport.ToggleGrid", "Toggle Grid", "Viewport", {KeyCode::G, false, false, false}, nullptr, 60,
        "Toggles viewport ground grid");
    RegisterAction("Viewport.Maximize", "Maximize Viewport", "Viewport", {KeyCode::Space, false, true, false}, nullptr,
        90, "Maximizes active viewport pane");
    RegisterAction("Viewport.PlayInEditor", "Play Simulation", "Viewport", {KeyCode::P, false, false, true}, nullptr,
        100, "Starts play-in-editor simulation");
    RegisterAction("Viewport.StopInEditor", "Stop Simulation", "Viewport", {KeyCode::Escape, false, false, false},
        nullptr, 100, "Stops active play-in-editor simulation");
    RegisterAction("Viewport.PerspectiveView", "Perspective View", "Viewport", {KeyCode::G, false, false, true},
        nullptr, 50, "Switches to 3D perspective camera");
    RegisterAction("Viewport.TopView", "Top View", "Viewport", {KeyCode::J, false, false, true}, nullptr, 50,
        "Switches to top orthographic view");
    RegisterAction("Viewport.FrontView", "Front View", "Viewport", {KeyCode::H, false, false, true}, nullptr, 50,
        "Switches to front orthographic view");
    RegisterAction("Viewport.SideView", "Side View", "Viewport", {KeyCode::K, false, false, true}, nullptr, 50,
        "Switches to side orthographic view");

    // --- Content Browser Category ---
    RegisterAction("ContentBrowser.Import", "Import Asset", "ContentBrowser", {KeyCode::I, true, false, false},
        nullptr, 70, "Imports external asset into project");
    RegisterAction("ContentBrowser.NewFolder", "New Folder", "ContentBrowser", {KeyCode::N, true, true, false},
        nullptr, 70, "Creates new folder in directory");
    RegisterAction("ContentBrowser.Open", "Open Asset", "ContentBrowser", {KeyCode::Enter, false, false, false},
        nullptr, 70, "Opens selected asset in editor");

    // --- Console Category ---
    RegisterAction("Console.Toggle", "Toggle Console", "Console", {KeyCode::Grave, false, false, false}, nullptr, 120,
        "Toggles developer debug console");
}

std::string HotkeyManager::ExportBindingsJson() const {
    std::stringstream ss;
    ss << "{\n  \"hotkeys\": {\n";
    size_t count = 0;
    for (const auto& [id, binding] : m_Bindings) {
        ss << "    \"" << id << "\": \"" << binding.chord.ToString() << "\"";
        if (++count < m_Bindings.size()) ss << ",";
        ss << "\n";
    }
    ss << "  }\n}\n";
    return ss.str();
}

bool HotkeyManager::ImportBindingsJson(const std::string& jsonContent) {
    std::stringstream ss(jsonContent);
    std::string line;
    bool inHotkeys = false;
    size_t reboundCount = 0;

    while (std::getline(ss, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.find("\"hotkeys\"") != std::string::npos) {
            inHotkeys = true;
            continue;
        }
        if (!inHotkeys) continue;

        size_t colon = trimmed.find(':');
        if (colon == std::string::npos) continue;

        std::string keyPart = trimmed.substr(0, colon);
        std::string valPart = trimmed.substr(colon + 1);

        size_t k1 = keyPart.find('"');
        size_t k2 = keyPart.rfind('"');
        size_t v1 = valPart.find('"');
        size_t v2 = valPart.rfind('"');

        if (k1 != std::string::npos && k2 > k1 && v1 != std::string::npos && v2 > v1) {
            std::string actionId = keyPart.substr(k1 + 1, k2 - k1 - 1);
            std::string chordStr = valPart.substr(v1 + 1, v2 - v1 - 1);
            HotkeyChord chord = HotkeyChord::FromString(chordStr);
            if (RebindAction(actionId, chord)) {
                reboundCount++;
            }
        }
    }

    WE_LOG_INFO(we::LogCategory::General.data(),
        "[HotkeyManager] Imported JSON configuration: Rebound " + std::to_string(reboundCount) + " actions");
    return reboundCount > 0;
}

bool HotkeyManager::LoadCustomBindings(const std::string& filePath) {
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) return false;
    std::stringstream ss;
    ss << inFile.rdbuf();
    return ImportBindingsJson(ss.str());
}

bool HotkeyManager::SaveCustomBindings(const std::string& filePath) const {
    std::ofstream outFile(filePath);
    if (!outFile.is_open()) return false;
    outFile << ExportBindingsJson();
    return true;
}

} // namespace we::runtime::kindui
