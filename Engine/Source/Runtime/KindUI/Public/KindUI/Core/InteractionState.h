#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Centralized interaction states. Widgets expose state; KindUI resolves appearance.
enum class InteractionState : uint32_t {
    Normal      = 0,
    Hovered     = 1 << 0,
    Pressed     = 1 << 1,
    Focused     = 1 << 2,
    Selected    = 1 << 3,
    Disabled    = 1 << 4,
    Checked     = 1 << 5,
    ReadOnly    = 1 << 6,
    Loading     = 1 << 7,
    Success     = 1 << 8,
    Warning     = 1 << 9,
    Error       = 1 << 10,
    Dragging    = 1 << 11,
    DropTarget  = 1 << 12,
};

inline InteractionState operator|(InteractionState a, InteractionState b) {
    return static_cast<InteractionState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline InteractionState operator&(InteractionState a, InteractionState b) {
    return static_cast<InteractionState>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasState(InteractionState flags, InteractionState test) {
    return (static_cast<uint32_t>(flags & test)) != 0;
}

struct InteractionStateSet {
    InteractionState flags = InteractionState::Normal;

    [[nodiscard]] bool IsHovered() const { return HasState(flags, InteractionState::Hovered); }
    [[nodiscard]] bool IsPressed() const { return HasState(flags, InteractionState::Pressed); }
    [[nodiscard]] bool IsFocused() const { return HasState(flags, InteractionState::Focused); }
    [[nodiscard]] bool IsSelected() const { return HasState(flags, InteractionState::Selected); }
    [[nodiscard]] bool IsDisabled() const { return HasState(flags, InteractionState::Disabled); }
    [[nodiscard]] bool IsChecked() const { return HasState(flags, InteractionState::Checked); }
    [[nodiscard]] bool IsReadOnly() const { return HasState(flags, InteractionState::ReadOnly); }
    [[nodiscard]] bool IsLoading() const { return HasState(flags, InteractionState::Loading); }
    [[nodiscard]] bool IsDragging() const { return HasState(flags, InteractionState::Dragging); }
    [[nodiscard]] bool IsDropTarget() const { return HasState(flags, InteractionState::DropTarget); }

    [[nodiscard]] bool IsInteractive() const {
        return !IsDisabled() && !IsReadOnly() && !IsLoading();
    }
};

} // namespace we::runtime::kindui
