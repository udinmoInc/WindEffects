#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

/// Explicit reference to a single WindIcons PNG asset.
/// stem: filename without extension, e.g. "icon_search"
/// sizePx: authored pixel size (16, 24, or 32)
struct WindIconRef {
    const char* stem = nullptr;
    uint32_t sizePx = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return stem != nullptr && stem[0] != '\0' && sizePx > 0;
    }
};

/// Asset stems available under Assets/Icons/WindIcons/.
namespace WindIconAssets {
    inline constexpr const char* Box = "icon_box";
    inline constexpr const char* Check = "icon_check";
    inline constexpr const char* ChevronDown = "icon_chevron-down";
    inline constexpr const char* ChevronLeft = "icon_chevron-left";
    inline constexpr const char* ChevronRight = "icon_chevron-right";
    inline constexpr const char* ChevronUp = "icon_chevron-up";
    inline constexpr const char* Close = "icon_close";
    inline constexpr const char* Eye = "icon_eye";
    inline constexpr const char* Globe = "icon_globe";
    inline constexpr const char* Grid3x3 = "icon_grid_3x3";
    inline constexpr const char* ListFilter = "icon_list_filter";
    inline constexpr const char* Minus = "icon_minus";
    inline constexpr const char* Redo = "icon_redo";
    inline constexpr const char* Refresh = "icon_refresh";
    inline constexpr const char* RefreshCwDot = "icon_refresh-cw-dot";
    inline constexpr const char* RoundedClose = "icon_rounded_close";
    inline constexpr const char* Scaling = "icon_scaling";
    inline constexpr const char* Search = "icon_search";
    inline constexpr const char* Square = "icon_square";
    inline constexpr const char* Sun = "icon_sun";
    inline constexpr const char* Undo = "icon_undo";
    inline constexpr const char* VerticalDots = "icon_vertical_dots";
    inline constexpr const char* Wrench = "icon_wrench";
} // namespace WindIconAssets

/// Invalid / blank icon slot.
inline constexpr WindIconRef kWindIconNone{ nullptr, 0 };

/// Explicit size presets — use at call sites; no automatic tier selection.
namespace WindIcons {
    inline constexpr WindIconRef Search16{ WindIconAssets::Search, 16 };
    inline constexpr WindIconRef Search24{ WindIconAssets::Search, 24 };
    inline constexpr WindIconRef Close16{ WindIconAssets::Close, 16 };
    inline constexpr WindIconRef RoundedClose16{ WindIconAssets::RoundedClose, 16 };
    inline constexpr WindIconRef ChevronDown16{ WindIconAssets::ChevronDown, 16 };
    inline constexpr WindIconRef ChevronRight16{ WindIconAssets::ChevronRight, 16 };
    inline constexpr WindIconRef ChevronLeft16{ WindIconAssets::ChevronLeft, 16 };
    inline constexpr WindIconRef ChevronUp16{ WindIconAssets::ChevronUp, 16 };
    inline constexpr WindIconRef Undo16{ WindIconAssets::Undo, 16 };
    inline constexpr WindIconRef Redo16{ WindIconAssets::Redo, 16 };
    inline constexpr WindIconRef Refresh16{ WindIconAssets::Refresh, 16 };
    inline constexpr WindIconRef RefreshCwDot16{ WindIconAssets::RefreshCwDot, 16 };
    inline constexpr WindIconRef Minus16{ WindIconAssets::Minus, 16 };
    inline constexpr WindIconRef Eye16{ WindIconAssets::Eye, 16 };
    inline constexpr WindIconRef Sun16{ WindIconAssets::Sun, 16 };
    inline constexpr WindIconRef Globe16{ WindIconAssets::Globe, 16 };
    inline constexpr WindIconRef Grid3x316{ WindIconAssets::Grid3x3, 16 };
    inline constexpr WindIconRef Wrench16{ WindIconAssets::Wrench, 16 };
    inline constexpr WindIconRef ListFilter16{ WindIconAssets::ListFilter, 16 };
    inline constexpr WindIconRef VerticalDots16{ WindIconAssets::VerticalDots, 16 };
    inline constexpr WindIconRef Check16{ WindIconAssets::Check, 16 };
    inline constexpr WindIconRef Scaling16{ WindIconAssets::Scaling, 16 };
    inline constexpr WindIconRef Box16{ WindIconAssets::Box, 16 };
    inline constexpr WindIconRef Square16{ WindIconAssets::Square, 16 };
}

} // namespace we::runtime::kindui
