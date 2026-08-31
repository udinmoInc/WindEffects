#!/usr/bin/env python3
"""Fix syntax damage from WindIcons migration."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "Engine" / "Source"


def fix_place_icon_draw(text: str) -> str:
    return re.sub(
        r"IconPainter::Draw\(([^;]+?), PlaceIconInControl\(([^)]+)\);",
        r"IconPainter::Draw(\1, PlaceIconInControl(\2));",
        text,
    )


def fix_glyph_centered_color(text: str) -> str:
    # Remove trailing color argument after PlaceGlyphCentered / CompactGlyphBand
    text = re.sub(
        r"(IconPainter::Draw\([^;]+?IconMetrics::(?:PlaceGlyphCentered|CompactGlyphBand)\([^)]+\)),\s*\n\s*[^;]+;",
        r"\1);",
        text,
        flags=re.DOTALL,
    )
    text = re.sub(
        r"(IconPainter::Draw\([^;]+?IconMetrics::(?:PlaceGlyphCentered|CompactGlyphBand)\([^)]+\)),\s*[^;]+;",
        r"\1);",
        text,
    )
    return text


def fix_toolbar_place_icon_color(text: str) -> str:
    text = re.sub(
        r"(IconPainter::Draw\([^;]+?ToolbarButtonChrome::PlaceIconInControl\([^)]+\)),\s*\n\s*[^;]+;",
        r"\1);",
        text,
        flags=re.DOTALL,
    )
    return text


def fix_rect_y_half(text: str) -> str:
    # Rect{ xExpr) * 0.5f, w, h } -> Rect{ xExpr, ??? }  handled per-pattern below
    patterns = [
        (
            r"Rect\{\s*([^)]+)\)\s*\*\s*0\.5f,\s*([^,]+),\s*([^}]+)\s*\}",
            None,  # skip generic - too risky
        ),
        (
            r"Rect\{\s*r\.x \+ \(r\.width - glyph\) \* 0\.5f\)\s*\*\s*0\.5f,\s*glyph,\s*glyph\s*\}",
            r"Rect{ r.x + (r.width - glyph) * 0.5f, r.y + (r.height - glyph) * 0.5f, glyph, glyph }",
        ),
        (
            r"Rect\{\s*r\.x \+ \(r\.width - iconSize\) \* 0\.5f\)\s*\*\s*0\.5f,\s*iconSize,\s*iconSize\s*\}",
            r"Rect{ r.x + (r.width - iconSize) * 0.5f, r.y + (r.height - iconSize) * 0.5f, iconSize, iconSize }",
        ),
        (
            r"Rect\{\s*m_Geometry\.x \+ pad\)\s*\*\s*0\.5f,\s*iconSize,\s*iconSize\s*\}",
            r"Rect{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f, iconSize, iconSize }",
        ),
        (
            r"Rect\{\s*row\.x \+ padX\)\s*\*\s*0\.5f,\s*iconSize,\s*iconSize\s*\}",
            r"Rect{ row.x + padX, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize }",
        ),
        (
            r"Rect\{\s*row\.x \+ row\.width - padX - iconSize\)\s*\*\s*0\.5f,\s*iconSize,\s*iconSize\s*\}",
            r"Rect{ row.x + row.width - padX - iconSize, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize }",
        ),
        (
            r"we::runtime::kindui::Rect\{\s*contentX\)\s*\*\s*0\.5f,\s*iconSize,\s*iconSize\s*\}",
            r"we::runtime::kindui::Rect{ contentX, button.rect.y + (button.rect.height - iconSize) * 0.5f, iconSize, iconSize }",
        ),
    ]
    for old, new in patterns:
        if new:
            text = text.replace(old, new) if not old.startswith("Rect") else re.sub(old, new, text)
    return text


def fix_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8", errors="replace")
    original = text

    text = fix_place_icon_draw(text)
    text = fix_glyph_centered_color(text)
    text = fix_toolbar_place_icon_color(text)
    text = fix_rect_y_half(text)

    # Remove duplicate WindIcon.h includes
    lines = text.splitlines(keepends=True)
    seen_windicon = False
    new_lines = []
    for line in lines:
        if '#include "KindUI/Core/WindIcon.h"' in line:
            if seen_windicon:
                continue
            seen_windicon = True
        new_lines.append(line)
    text = "".join(new_lines)

    if text != original:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> None:
    count = 0
    for path in ROOT.rglob("*"):
        if path.suffix not in {".cpp", ".h"}:
            continue
        if fix_file(path):
            count += 1
            print(path.relative_to(ROOT.parent.parent))
    print(f"fixed {count} files")


if __name__ == "__main__":
    main()
