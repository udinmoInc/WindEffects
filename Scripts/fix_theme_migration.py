import os
import re

ROOT = os.path.join(os.path.dirname(__file__), "..", "Engine", "Source")

FILES_WITH_USING_THEME = [
    "Editor/ToolsPanel/Private/Widgets/EditorModeSelector.cpp",
    "Editor/ToolsPanel/Private/Widgets/ToolsPanel.cpp",
    "Editor/PlaceActors/Private/PlaceActors/PlaceActorsItem.cpp",
    "Editor/PlaceActors/Private/PlaceActors/PlaceActorsPanel.cpp",
    "Editor/PlaceActors/Private/PlaceActors/PlaceActorsCategory.cpp",
    "Editor/Viewport/Private/Widgets/ViewportSliderPopup.cpp",
]

STATIC_THEME_FILES = [
    "Editor/PlaceActors/Private/PlaceActors/PlaceActorsItem.cpp",
    "Editor/PlaceActors/Private/PlaceActors/PlaceActorsCategory.cpp",
    "Editor/ContentBrowser/Private/Widgets/TreeView.cpp",
    "Editor/Toolbar/Private/Widgets/ToolButton.cpp",
    "Editor/ContentBrowser/Private/Widgets/ContentBrowserToolbar.cpp",
    "Editor/ContentBrowser/Private/Services/ThumbnailRenderer.cpp",
]


def ensure_include(text: str, include_line: str) -> str:
    if include_line in text:
        return text
    if "#include" in text:
        return re.sub(
            r'(#include[^\n]+\n)',
            r'\1' + include_line + "\n",
            text,
            count=1,
        )
    return include_line + "\n" + text


def fix_using_theme(path: str, text: str) -> str:
    text = text.replace(
        "using WindEffects::Editor::UI::Theme;",
        "using WindEffects::Editor::UI::ThemeToken;",
    )
    text = ensure_include(text, '#include "WindEffects/Editor/UI/Theming/ThemeToken.h"')
    return text


def fix_static_theme(path: str, text: str) -> str:
    text = ensure_include(text, '#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"')
    replacements = [
        ("ThemeInteractiveBackground(", "ResolveThemeInteractiveBackground("),
        ("ThemeTextForState(", "ResolveThemeTextForState("),
        ("ThemeIconForState(", "ResolveThemeIconForState("),
        ("ThemeColor(", "ResolveThemeColor("),
        ("ThemeMetric(", "ResolveThemeMetric("),
        ("ThemePadding(", "ResolveThemePadding("),
    ]
    for old, new in replacements:
        text = text.replace(old, new)
    return text


def main() -> None:
    changed = []
    for rel in set(FILES_WITH_USING_THEME + STATIC_THEME_FILES):
        path = os.path.join(ROOT, rel.replace("/", os.sep))
        if not os.path.exists(path):
            continue
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
        orig = text
        if rel in FILES_WITH_USING_THEME:
            text = fix_using_theme(path, text)
        if rel in STATIC_THEME_FILES:
            text = fix_static_theme(path, text)
        if text != orig:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text)
            changed.append(rel)

    print(f"Fixed {len(changed)} files")
    for rel in changed:
        print(rel)


if __name__ == "__main__":
    main()
