#include "Platform/InputTypes.h"

#include <array>
#include <cstring>

namespace we::platform {
namespace {

struct KeyName {
    KeyCode code;
    const char* name;
};

constexpr KeyName kKeyNames[] = {
    {KeyCode::A, "A"}, {KeyCode::B, "B"}, {KeyCode::C, "C"}, {KeyCode::D, "D"},
    {KeyCode::E, "E"}, {KeyCode::F, "F"}, {KeyCode::G, "G"}, {KeyCode::H, "H"},
    {KeyCode::I, "I"}, {KeyCode::J, "J"}, {KeyCode::K, "K"}, {KeyCode::L, "L"},
    {KeyCode::M, "M"}, {KeyCode::N, "N"}, {KeyCode::O, "O"}, {KeyCode::P, "P"},
    {KeyCode::Q, "Q"}, {KeyCode::R, "R"}, {KeyCode::S, "S"}, {KeyCode::T, "T"},
    {KeyCode::U, "U"}, {KeyCode::V, "V"}, {KeyCode::W, "W"}, {KeyCode::X, "X"},
    {KeyCode::Y, "Y"}, {KeyCode::Z, "Z"},
    {KeyCode::Escape, "Escape"}, {KeyCode::Tab, "Tab"}, {KeyCode::Space, "Space"},
    {KeyCode::Enter, "Enter"}, {KeyCode::Backspace, "Backspace"},
    {KeyCode::Left, "Left"}, {KeyCode::Right, "Right"}, {KeyCode::Up, "Up"}, {KeyCode::Down, "Down"},
    {KeyCode::LeftShift, "LeftShift"}, {KeyCode::RightShift, "RightShift"},
    {KeyCode::LeftControl, "LeftControl"}, {KeyCode::RightControl, "RightControl"},
    {KeyCode::LeftAlt, "LeftAlt"}, {KeyCode::RightAlt, "RightAlt"},
};

} // namespace

const char* ToString(KeyCode key) noexcept {
    for (const auto& entry : kKeyNames) {
        if (entry.code == key) {
            return entry.name;
        }
    }
    return "Unknown";
}

KeyCode KeyCodeFromName(std::string_view name) noexcept {
    for (const auto& entry : kKeyNames) {
        if (name == entry.name) {
            return entry.code;
        }
    }
    return KeyCode::Unknown;
}

} // namespace we::platform
