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
    inline constexpr const char* AdjustmentHorizontal = "icon_adjustment-horizontal";
    inline constexpr const char* AlertNormal = "icon_alert-normal";
    inline constexpr const char* AlertRed = "icon_alert_red";
    inline constexpr const char* Box = "icon_box";
    inline constexpr const char* BoxSolid = "icon_box-solid";
    inline constexpr const char* Bulb = "icon_bulb";
    inline constexpr const char* Camera = "icon_camera";
    inline constexpr const char* Check = "icon_check";
    inline constexpr const char* ChevronDown = "icon_chevron-down";
    inline constexpr const char* ChevronLeft = "icon_chevron-left";
    inline constexpr const char* ChevronRight = "icon_chevron-right";
    inline constexpr const char* ChevronUp = "icon_chevron-up";
    inline constexpr const char* CircleHelp = "icon_circle-help";
    inline constexpr const char* Close = "icon_close";
    inline constexpr const char* Cloud = "icon_cloud";
    inline constexpr const char* Eye = "icon_eye";
    inline constexpr const char* FolderClosed = "icon_folder-closed";
    inline constexpr const char* FolderOpened = "icon_folder-opened";
    inline constexpr const char* FolderPlus = "icon_folder-plus";
    inline constexpr const char* FolderSearch = "icon_folder-search";
    inline constexpr const char* Globe = "icon_globe";
    inline constexpr const char* Grid3x3 = "icon_grid_3x3";
    inline constexpr const char* InfoNormal = "icon_info_normal";
    inline constexpr const char* InfoYellow = "icon_info_yellow";
    inline constexpr const char* ListFilter = "icon_list_filter";
    inline constexpr const char* Lock = "icon_lock";
    inline constexpr const char* LockOpen = "icon_lock-open";
    inline constexpr const char* Logs = "icon_logs";
    inline constexpr const char* Minus = "icon_minus";
    inline constexpr const char* Mouse = "icon_mouse";
    inline constexpr const char* PlayGreen = "icon_play-green";
    inline constexpr const char* Plus = "icon_plus";
    inline constexpr const char* PlusCircle = "icon_plus-circle";
    inline constexpr const char* Redo = "icon_redo";
    inline constexpr const char* RedoAlt = "icon_redo-alt";
    inline constexpr const char* Refresh = "icon_refresh";
    inline constexpr const char* RefreshCwDot = "icon_refresh-cw-dot";
    inline constexpr const char* RoundedClose = "icon_rounded_close";
    inline constexpr const char* Scaling = "icon_scaling";
    inline constexpr const char* Search = "icon_search";
    inline constexpr const char* Settings = "icon_settings";
    inline constexpr const char* Square = "icon_square";
    inline constexpr const char* StopSolid = "icon_stop-solid";
    inline constexpr const char* Sun = "icon_sun";
    inline constexpr const char* Undo = "icon_undo";
    inline constexpr const char* VerticalDots = "icon_vertical_dots";
    inline constexpr const char* VideoCamera = "icon_video-camera";
    inline constexpr const char* WorldGlobe = "icon_world-globe";
    inline constexpr const char* Wrench = "icon_wrench";
} // namespace WindIconAssets

/// Invalid / blank icon slot.
inline constexpr WindIconRef kWindIconNone{ nullptr, 0 };

/// Explicit size presets — use at call sites; no automatic tier selection.
namespace WindIcons {
    inline constexpr WindIconRef AdjustmentHorizontal16{ WindIconAssets::AdjustmentHorizontal, 16 };
    inline constexpr WindIconRef AlertNormal16{ WindIconAssets::AlertNormal, 16 };
    inline constexpr WindIconRef AlertRed16{ WindIconAssets::AlertRed, 16 };
    inline constexpr WindIconRef Box16{ WindIconAssets::Box, 16 };
    inline constexpr WindIconRef BoxSolid16{ WindIconAssets::BoxSolid, 16 };
    inline constexpr WindIconRef Bulb16{ WindIconAssets::Bulb, 16 };
    inline constexpr WindIconRef Camera16{ WindIconAssets::Camera, 16 };
    inline constexpr WindIconRef Check16{ WindIconAssets::Check, 16 };
    inline constexpr WindIconRef ChevronDown16{ WindIconAssets::ChevronDown, 16 };
    inline constexpr WindIconRef ChevronLeft16{ WindIconAssets::ChevronLeft, 16 };
    inline constexpr WindIconRef ChevronRight16{ WindIconAssets::ChevronRight, 16 };
    inline constexpr WindIconRef ChevronUp16{ WindIconAssets::ChevronUp, 16 };
    inline constexpr WindIconRef CircleHelp16{ WindIconAssets::CircleHelp, 16 };
    inline constexpr WindIconRef Close16{ WindIconAssets::Close, 16 };
    inline constexpr WindIconRef Cloud16{ WindIconAssets::Cloud, 16 };
    inline constexpr WindIconRef Eye16{ WindIconAssets::Eye, 16 };
    inline constexpr WindIconRef FolderClosed16{ WindIconAssets::FolderClosed, 16 };
    inline constexpr WindIconRef FolderOpened16{ WindIconAssets::FolderOpened, 16 };
    inline constexpr WindIconRef FolderPlus16{ WindIconAssets::FolderPlus, 16 };
    inline constexpr WindIconRef FolderSearch16{ WindIconAssets::FolderSearch, 16 };
    inline constexpr WindIconRef Globe16{ WindIconAssets::Globe, 16 };
    inline constexpr WindIconRef Grid3x316{ WindIconAssets::Grid3x3, 16 };
    inline constexpr WindIconRef InfoNormal16{ WindIconAssets::InfoNormal, 16 };
    inline constexpr WindIconRef InfoYellow16{ WindIconAssets::InfoYellow, 16 };
    inline constexpr WindIconRef ListFilter16{ WindIconAssets::ListFilter, 16 };
    inline constexpr WindIconRef Lock16{ WindIconAssets::Lock, 16 };
    inline constexpr WindIconRef LockOpen16{ WindIconAssets::LockOpen, 16 };
    inline constexpr WindIconRef Logs16{ WindIconAssets::Logs, 16 };
    inline constexpr WindIconRef Minus16{ WindIconAssets::Minus, 16 };
    inline constexpr WindIconRef Mouse16{ WindIconAssets::Mouse, 16 };
    inline constexpr WindIconRef PlayGreen16{ WindIconAssets::PlayGreen, 16 };
    inline constexpr WindIconRef Plus16{ WindIconAssets::Plus, 16 };
    inline constexpr WindIconRef PlusCircle16{ WindIconAssets::PlusCircle, 16 };
    inline constexpr WindIconRef Redo16{ WindIconAssets::Redo, 16 };
    inline constexpr WindIconRef RedoAlt16{ WindIconAssets::RedoAlt, 16 };
    inline constexpr WindIconRef Refresh16{ WindIconAssets::Refresh, 16 };
    inline constexpr WindIconRef RefreshCwDot16{ WindIconAssets::RefreshCwDot, 16 };
    inline constexpr WindIconRef RoundedClose16{ WindIconAssets::RoundedClose, 16 };
    inline constexpr WindIconRef Scaling16{ WindIconAssets::Scaling, 16 };
    inline constexpr WindIconRef Search16{ WindIconAssets::Search, 16 };
    inline constexpr WindIconRef Settings16{ WindIconAssets::Settings, 16 };
    inline constexpr WindIconRef Square16{ WindIconAssets::Square, 16 };
    inline constexpr WindIconRef StopSolid16{ WindIconAssets::StopSolid, 16 };
    inline constexpr WindIconRef Sun16{ WindIconAssets::Sun, 16 };
    inline constexpr WindIconRef Undo16{ WindIconAssets::Undo, 16 };
    inline constexpr WindIconRef VerticalDots16{ WindIconAssets::VerticalDots, 16 };
    inline constexpr WindIconRef VideoCamera16{ WindIconAssets::VideoCamera, 16 };
    inline constexpr WindIconRef WorldGlobe16{ WindIconAssets::WorldGlobe, 16 };
    inline constexpr WindIconRef Wrench16{ WindIconAssets::Wrench, 16 };
} // namespace WindIcons

} // namespace we::runtime::kindui
