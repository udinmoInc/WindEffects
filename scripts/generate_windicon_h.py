#!/usr/bin/env python3
"""Generate WindIcon.h from Assets/Icons/WindIcons/*.png stems."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ICONS_DIR = ROOT / "Assets" / "Icons" / "WindIcons"
OUT_PATH = ROOT / "Engine" / "Source" / "Runtime" / "KindUI" / "Public" / "KindUI" / "Core" / "WindIcon.h"


def stem_to_cpp_name(stem: str) -> str:
    parts = re.split(r"[-_]", stem)
    return "".join(p[0].upper() + p[1:] if p else "" for p in parts)


def collect_stems() -> list[str]:
    stems: set[str] = set()
    for path in ICONS_DIR.glob("icon_*.png"):
        match = re.match(r"icon_(.+)_\d+$", path.stem)
        if match:
            stems.add(match.group(1))
    return sorted(stems)


def generate_header(stems: list[str]) -> str:
    lines: list[str] = [
        "#pragma once",
        "",
        '#include "KindUI/Export.h"',
        "",
        "#include <cstdint>",
        "",
        "namespace we::runtime::kindui {",
        "",
        "/// Explicit reference to a single WindIcons PNG asset.",
        '/// stem: filename without extension, e.g. "icon_search"',
        "/// sizePx: authored pixel size (16, 24, or 32)",
        "struct WindIconRef {",
        "    const char* stem = nullptr;",
        "    uint32_t sizePx = 0;",
        "",
        "    [[nodiscard]] constexpr bool IsValid() const noexcept {",
        "        return stem != nullptr && stem[0] != '\\0' && sizePx > 0;",
        "    }",
        "};",
        "",
        "/// Asset stems available under Assets/Icons/WindIcons/.",
        "namespace WindIconAssets {",
    ]

    for stem in stems:
        cpp = stem_to_cpp_name(stem)
        lines.append(f'    inline constexpr const char* {cpp} = "icon_{stem}";')

    lines.extend(
        [
            "} // namespace WindIconAssets",
            "",
            "/// Invalid / blank icon slot.",
            "inline constexpr WindIconRef kWindIconNone{ nullptr, 0 };",
            "",
            "/// Explicit size presets — use at call sites; no automatic tier selection.",
            "namespace WindIcons {",
        ]
    )

    for stem in stems:
        cpp = stem_to_cpp_name(stem)
        lines.append(f"    inline constexpr WindIconRef {cpp}16{{ WindIconAssets::{cpp}, 16 }};")

    lines.extend(
        [
            "} // namespace WindIcons",
            "",
            "} // namespace we::runtime::kindui",
            "",
        ]
    )

    return "\n".join(lines)


def main() -> int:
    stems = collect_stems()
    OUT_PATH.write_text(generate_header(stems), encoding="utf-8")
    print(f"Wrote {OUT_PATH} with {len(stems)} icon assets")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
