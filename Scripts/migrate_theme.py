import re
import os
import glob

ROOT = os.path.join(os.path.dirname(__file__), "..", "Engine", "Source")

COLOR_FIELDS = [
    "WindowBackground", "WorkspaceBackground", "ToolbarBackground", "PanelBackground",
    "HeaderBackground", "ViewportBackground", "FooterBackground", "MenuBarBackground",
    "TabBackground", "PopupBackground", "ContentBrowserBackground",
    "BorderLight", "BorderDark", "BorderDefault", "BorderFocus", "Separator",
    "HoverBackground", "ActiveBackground", "SelectedBackground", "DisabledBackground", "PressedBackground",
    "TextPrimary", "TextSecondary", "TextDisabled", "TextMuted", "TextWindowLabel",
    "IconDefault", "IconHover", "IconActive", "IconDisabled",
    "AccentPrimary", "AccentHover", "ButtonPrimaryBackground", "ButtonPrimaryHover", "ButtonPrimaryPressed",
    "ActiveTabLine", "SelectionHighlight", "Success", "Warning",
    "InputBackground", "SearchBoxBackground", "SearchPlaceholder",
    "ViewportToolbarBackground", "StatusBarBackground",
    "GizmoBackground", "GizmoAxisX", "GizmoAxisY", "GizmoAxisZ",
    "HighlightSubtle", "ShadowSubtle", "ShadowOverlay", "ModalScrim",
    "ContentBrowserHoverBackground", "ContentBrowserFolderShadow", "ContentBrowserFolderEdge",
    "ContentBrowserFolderHighlight", "ContentBrowserFolderTab", "ContentBrowserFolderPrimary", "ContentBrowserFolderBody",
    "CloseButtonHover", "DragGhostBackground", "TooltipBackground",
    "ErrorForeground", "LinkForeground", "CodeForeground",
    "HoverBg", "SelectedBg", "SearchBoxBg", "PressedOverlay", "HoverOverlay", "HoverMenu", "HoverButton",
    "IconMuted", "SearchIcon", "SelectedAccent", "BorderSecondary", "ContentBrowserHoverBg", "ContentBrowserItemLabel",
    "SidebarIconDefault", "SidebarIconHover", "TreeArrow",
]

METRIC_FIELDS = [
    "CornerRadiusSmall", "CornerRadiusMedium", "CornerRadiusLarge", "WindowCornerRadius",
    "TextSizeMenu", "TextSizeToolbar", "TextSizeTabs", "TextSizeNormal", "TextSizeProperty",
    "TextSizeCaption", "TextSizeWindow", "TextSizeHeader", "TextSizeBody", "TextSizeSmall",
    "BorderWidth", "PanelHeaderHeight", "ToolbarHeight", "SearchBoxHeight",
    "IconButtonSize", "ButtonHeight", "NavigationButtonSize",
    "IconSizeSearch", "IconSizeToolbar", "IconSizeTree", "IconSizeNavigation",
    "IconButtonRadius", "ButtonPaddingHorizontal", "ButtonSpacing", "ButtonGroupSpacing",
    "Space1", "Space2", "Space3", "Space4", "Space5", "Space6",
    "HoverAnimationDamping", "PressAnimationDamping", "PressOffset",
    "ShadowBlurSmall", "ShadowBlurMedium", "ShadowSpreadMedium",
]

ALIAS = {
    "HoverBg": "HoverBackground",
    "SelectedBg": "SelectedBackground",
    "SearchBoxBg": "SearchBoxBackground",
    "PressedOverlay": "PressedBackground",
    "HoverOverlay": "HoverBackground",
    "HoverMenu": "HoverBackground",
    "HoverButton": "HoverBackground",
    "IconMuted": "IconDefault",
    "SearchIcon": "IconDefault",
    "SelectedAccent": "AccentPrimary",
    "BorderSecondary": "BorderDark",
    "ContentBrowserHoverBg": "ContentBrowserHoverBackground",
    "ContentBrowserItemLabel": "TextPrimary",
    "SidebarIconDefault": "IconDefault",
    "SidebarIconHover": "IconHover",
    "TreeArrow": "TextSecondary",
}


def token_for(field: str) -> str:
    return ALIAS.get(field, field)


def migrate_file(path: str) -> bool:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    orig = text

    text = text.replace('#include "Core/Theme.h"', '#include "WindEffects/Editor/UI/Theming/ThemeToken.h"')
    text = text.replace('#include "Core/ToolbarDesignTokens.h"', '#include "WindEffects/Editor/UI/Theming/ThemeToken.h"')

    for field in COLOR_FIELDS:
        tok = token_for(field)
        text = re.sub(rf"Theme::Get\(\)\.{field}\b", f"ThemeColor(ThemeToken::{tok})", text)
        text = re.sub(rf"\btheme\.{field}\b", f"ThemeColor(ThemeToken::{tok})", text)

    for field in METRIC_FIELDS:
        tok = token_for(field)
        text = re.sub(rf"Theme::Get\(\)\.{field}\b", f"ThemeMetric(ThemeToken::{tok})", text)
        text = re.sub(rf"\btheme\.{field}\b", f"ThemeMetric(ThemeToken::{tok})", text)

    text = re.sub(r"\btheme\.InteractiveBackground\(", "ThemeInteractiveBackground(", text)
    text = re.sub(r"\btheme\.TextForState\(", "ThemeTextForState(", text)
    text = re.sub(r"\btheme\.IconForState\(", "ThemeIconForState(", text)
    text = re.sub(r"Theme::Get\(\)\.InteractiveBackground\(", "ThemeInteractiveBackground(", text)
    text = re.sub(r"Theme::Get\(\)\.TextForState\(", "ThemeTextForState(", text)
    text = re.sub(r"Theme::Get\(\)\.IconForState\(", "ThemeIconForState(", text)
    text = re.sub(r"\s*const auto& theme = Theme::Get\(\);\r?\n", "\n", text)

    if text != orig:
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
        return True
    return False


def main() -> None:
    skip_parts = ["Theme.cpp", "Theme.h", "ToolbarDesignTokens.h", "GraphiteDarkTheme.cpp", "AssetRegistry.cpp"]
    patterns = ["Editor/**/*.cpp", "Editor/**/*.h", "Programs/Editor/**/*.cpp"]
    files: list[str] = []
    for pattern in patterns:
        files.extend(glob.glob(os.path.join(ROOT, pattern), recursive=True))

    changed: list[str] = []
    for path in files:
        norm = path.replace("\\", "/")
        if any(part in norm for part in skip_parts):
            continue
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
        if "Theme::Get" not in content and "Core/Theme.h" not in content and "ToolbarDesignTokens" not in content:
            continue
        if migrate_file(path):
            changed.append(path)

    print(f"Changed {len(changed)} files")
    for path in changed:
        print(os.path.relpath(path, ROOT))


if __name__ == "__main__":
    main()
