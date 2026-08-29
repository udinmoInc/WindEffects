#pragma once

#include "KindUI/Core/Types.h"

namespace we::runtime::kindui::palette {

// Authoritative Graphite Dark palette — one canonical value per unique color.
// ColorToken::ResolveColor maps semantic roles onto these entries.
// Do not duplicate HEX values elsewhere in the UI stack.

struct GraphiteDark {
    // ── Surfaces (darkest → lightest) ───────────────────────────────────────
    static constexpr Color Window       = {0x15 / 255.0f, 0x15 / 255.0f, 0x15 / 255.0f, 1.0f}; // #151515
    static constexpr Color ScrollbarTrack = {0x1A / 255.0f, 0x1A / 255.0f, 0x1A / 255.0f, 1.0f}; // #1A1A1A
    static constexpr Color Panel        = {0x24 / 255.0f, 0x24 / 255.0f, 0x24 / 255.0f, 1.0f}; // #242424
    static constexpr Color Secondary    = {0x1D / 255.0f, 0x1E / 255.0f, 0x20 / 255.0f, 1.0f}; // #1D1E20
    static constexpr Color Card         = {0x22 / 255.0f, 0x23 / 255.0f, 0x26 / 255.0f, 1.0f}; // #222326
    static constexpr Color Header       = {0x1C / 255.0f, 0x1D / 255.0f, 0x1F / 255.0f, 1.0f}; // #1C1D1F
    static constexpr Color Input        = {0x14 / 255.0f, 0x15 / 255.0f, 0x17 / 255.0f, 1.0f}; // #141517
    static constexpr Color TabInactive    = {0x29 / 255.0f, 0x2A / 255.0f, 0x2D / 255.0f, 1.0f}; // #292A2D
    static constexpr Color Disabled     = {0x11 / 255.0f, 0x12 / 255.0f, 0x14 / 255.0f, 1.0f}; // #111214

    // ── Interaction states ──────────────────────────────────────────────────
    static constexpr Color Hover        = {0x27 / 255.0f, 0x28 / 255.0f, 0x2B / 255.0f, 1.0f}; // #27282B
    static constexpr Color Pressed      = {0x14 / 255.0f, 0x15 / 255.0f, 0x17 / 255.0f, 1.0f}; // #141517
    static constexpr Color Selected     = {0x32 / 255.0f, 0x33 / 255.0f, 0x36 / 255.0f, 1.0f}; // #323336
    static constexpr Color ControlHover   = {0x1B / 255.0f, 0x1C / 255.0f, 0x1F / 255.0f, 1.0f}; // #1B1C1F
    static constexpr Color ControlSelected = {0x24 / 255.0f, 0x27 / 255.0f, 0x2B / 255.0f, 1.0f}; // #24272B

    // ── Borders & separators ────────────────────────────────────────────────
    static constexpr Color Separator    = {0x23 / 255.0f, 0x24 / 255.0f, 0x27 / 255.0f, 1.0f}; // #232427
    static constexpr Color Border       = {0x29 / 255.0f, 0x2A / 255.0f, 0x2D / 255.0f, 1.0f}; // #292A2D
    static constexpr Color BorderLight    = {0x38 / 255.0f, 0x39 / 255.0f, 0x3C / 255.0f, 1.0f}; // #38393C

    // ── Text ────────────────────────────────────────────────────────────────
    static constexpr Color Text         = {0xE6 / 255.0f, 0xE6 / 255.0f, 0xE6 / 255.0f, 1.0f}; // #E6E6E6
    static constexpr Color TextSecondary = {0xA0 / 255.0f, 0xA1 / 255.0f, 0xA3 / 255.0f, 1.0f}; // #A0A1A3
    static constexpr Color TextHint     = {0x70 / 255.0f, 0x71 / 255.0f, 0x74 / 255.0f, 1.0f}; // #707174
    static constexpr Color TextDisabled = {0x4F / 255.0f, 0x50 / 255.0f, 0x53 / 255.0f, 1.0f}; // #4F5053

    // ── Accent & semantic ───────────────────────────────────────────────────
    static constexpr Color Accent       = {0x38 / 255.0f, 0x39 / 255.0f, 0x3C / 255.0f, 1.0f}; // #38393C
    static constexpr Color AccentHover    = {0x40 / 255.0f, 0x44 / 255.0f, 0x4A / 255.0f, 1.0f}; // #40444A
    static constexpr Color Success      = {0x4C / 255.0f, 0xAF / 255.0f, 0x50 / 255.0f, 1.0f}; // #4CAF50
    static constexpr Color Warning      = {0xD6 / 255.0f, 0xA6 / 255.0f, 0x2A / 255.0f, 1.0f}; // #D6A62A
    static constexpr Color Error        = {0xE0 / 255.0f, 0x52 / 255.0f, 0x52 / 255.0f, 1.0f}; // #E05252

    // ── Content-browser folder art ──────────────────────────────────────────
    static constexpr Color FolderBody   = {0x2D / 255.0f, 0x2E / 255.0f, 0x31 / 255.0f, 1.0f}; // #2D2E31
    static constexpr Color FolderPrimary = {0x32 / 255.0f, 0x33 / 255.0f, 0x36 / 255.0f, 1.0f}; // #323336

    // ── Scrollbar ───────────────────────────────────────────────────────────
    static constexpr Color ScrollThumb  = {0x58 / 255.0f, 0x58 / 255.0f, 0x58 / 255.0f, 1.0f}; // #585858
    static constexpr Color ScrollThumbHover = {0x6E / 255.0f, 0x6E / 255.0f, 0x6E / 255.0f, 1.0f}; // #6E6E6E

    // ── Depth / overlays (alpha variants) ───────────────────────────────────
    static constexpr Color HighlightSubtle = {1.0f, 1.0f, 1.0f, 0.05f};
    static constexpr Color ShadowSubtle   = {0.0f, 0.0f, 0.0f, 0.22f};
    static constexpr Color ShadowOverlay  = {0.0f, 0.0f, 0.0f, 0.42f};
    static constexpr Color ShadowPopup    = {0.0f, 0.0f, 0.0f, 0.32f};
    static constexpr Color ShadowColor    = {0.0f, 0.0f, 0.0f, 0.28f};
    static constexpr Color ModalScrim     = {0.0f, 0.0f, 0.0f, 0.55f};
    static constexpr Color FolderShadow   = {0.0f, 0.0f, 0.0f, 0.38f};
    static constexpr Color TooltipBg      = {0x22 / 255.0f, 0x23 / 255.0f, 0x26 / 255.0f, 0.97f};
    static constexpr Color DragGhost      = {0x22 / 255.0f, 0x23 / 255.0f, 0x26 / 255.0f, 0.90f};
    static constexpr Color ViewportToolbar = {0x1C / 255.0f, 0x1D / 255.0f, 0x1F / 255.0f, 0.96f};
    static constexpr Color ActiveTabLine  = {0x38 / 255.0f, 0x39 / 255.0f, 0x3C / 255.0f, 0.80f};
    static constexpr Color SelectionHighlight = {0x32 / 255.0f, 0x33 / 255.0f, 0x36 / 255.0f, 0.90f};

    // ── Gizmo axes ──────────────────────────────────────────────────────────
    static constexpr Color GizmoAxisX     = {0.90f, 0.25f, 0.25f, 1.0f};
    static constexpr Color GizmoAxisY     = {0.30f, 0.85f, 0.35f, 1.0f};
    static constexpr Color GizmoAxisZ     = {0.30f, 0.50f, 0.95f, 1.0f};

    // ── Button danger (unique shades) ─────────────────────────────────────────
    static constexpr Color DangerHover    = {0xEC / 255.0f, 0x64 / 255.0f, 0x64 / 255.0f, 1.0f};
    static constexpr Color DangerPressed  = {0xC0 / 255.0f, 0x3A / 255.0f, 0x3A / 255.0f, 1.0f};
};

} // namespace we::runtime::kindui::palette
