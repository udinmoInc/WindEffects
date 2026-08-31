#pragma once

#include "Icons/Export.h"

#include <array>
#include <cstdint>

namespace we::runtime::icons::kindicons {

/// One runtime atlas icon name ↔ one kindicons source stem (`icon_<stem>__<tier>.png`).
struct KindIconBinding {
    const char* runtimeName;
    const char* sourceStem;
};

/// Supported kindicon export tiers (overlay applies only when the PNG exists).
inline constexpr std::array<std::uint32_t, 7> kTiers = {12, 16, 20, 24, 32, 48, 64};

/// Manually curated — add entries here, then re-run `we asset icons`.
/// Do not auto-discover icons from the kindicons folder.
inline constexpr KindIconBinding kBindings[] = {
    // Original bindings
    {"search",      "search"},
    {"settings",    "settings"},
    {"star",        "star"},
    {"object",      "object"},
    {"hand",        "hand"},
    {"alert",       "alert"},
    {"warning",     "warning"},
    {"question",    "question"},
    {"block",       "block"},
    // New icons added from kindicons/icons
    {"sun",         "Sun"},         // icon_Sun__*.png  → Icons_Sun atlas slot
    {"sun",         "Globe"},       // icon_Globe__*.png → same slot; Globe art wins (listed after Sun)
    {"chevrondown", "chevrondown"}, // icon_chevrondown__*.png
    {"chevronleft", "chevronleft"}, // icon_chevronleft__*.png
    {"folder",      "folder"},      // icon_folder__*.png
    {"openfolder",  "folderopened"},// icon_folderopened__*.png → Icons_OpenFolder slot
    {"redo",        "refresh"},     // icon_refresh__*.png → Icons_Redo slot (refresh art replaces redo glyph)
    {"save",        "save"},        // icon_save__*.png
    {"lock",        "lock"},        // icon_lock__*.png (no atlas region yet — no-op until atlas updated)
    {"unlock",      "unlocked"},    // icon_unlocked__*.png (no atlas region yet — no-op until atlas updated)
};

} // namespace we::runtime::icons::kindicons
