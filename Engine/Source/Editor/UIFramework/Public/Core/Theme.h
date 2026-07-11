#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Core/Geometry.h"

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API Theme {
public:
    Color WindowBackground{};
    Color WorkspaceBackground{};
    Color ToolbarBackground{};
    Color PanelBackground{};
    Color HeaderBackground{};
    Color ViewportBackground{};
    Color FooterBackground{};
    Color MenuBarBackground{};
    Color TabBackground{};
    Color PopupBackground{};
    Color ContentBrowserBackground{};

    Color BorderLight{};
    Color BorderDark{};
    Color BorderDefault{};
    Color BorderSecondary{};
    Color BorderFocus{};
    Color Separator{};

    Color HoverBackground{};
    Color HoverBg{};
    Color HoverOverlay{};
    Color HoverMenu{};
    Color HoverButton{};
    Color ActiveBackground{};
    Color SelectedBackground{};
    Color SelectedBg{};
    Color DisabledBackground{};
    Color PressedBackground{};
    Color PressedOverlay{};

    Color TextPrimary{};
    Color TextSecondary{};
    Color TextDisabled{};
    Color TextMuted{};
    Color TextWindowLabel{};

    Color IconDefault{};
    Color IconHover{};
    Color IconActive{};
    Color IconDisabled{};
    Color IconMuted{};
    Color SearchIcon{};

    Color AccentPrimary{};
    Color AccentHover{};
    Color ButtonPrimaryBackground{};
    Color ButtonPrimaryHover{};
    Color ButtonPrimaryPressed{};
    Color ActiveTabLine{};
    Color SelectionHighlight{};
    Color SelectedAccent{};
    Color Success{};
    Color Warning{};

    Color InputBackground{};
    Color SearchBoxBackground{};
    Color SearchBoxBg{};
    Color SearchPlaceholder{};

    Color ViewportToolbarBackground{};
    Color StatusBarBackground{};

    Color ContentBrowserHoverBg{};
    Color ContentBrowserItemLabel{};
    Color ContentBrowserFolderShadow{};
    Color ContentBrowserFolderEdge{};
    Color ContentBrowserFolderHighlight{};
    Color ContentBrowserFolderTab{};
    Color ContentBrowserFolderPrimary{};
    Color ContentBrowserFolderBody{};
    Color SidebarIconDefault{};
    Color SidebarIconHover{};
    Color TreeArrow{};

    Color GizmoBackground{};
    Color GizmoAxisX{};
    Color GizmoAxisY{};
    Color GizmoAxisZ{};

    Color HighlightSubtle{};
    Color ShadowSubtle{};

    float CornerRadiusSmall = 4.0f;
    float CornerRadiusMedium = 6.0f;
    float CornerRadiusLarge = 6.0f;
    float WindowCornerRadius = 10.0f;

    float TextSizeMenu = 13.0f;
    float TextSizeToolbar = 13.0f;
    float TextSizeTabs = 13.0f;
    float TextSizeNormal = 13.0f;
    float TextSizeBody = 13.0f;
    float TextSizeProperty = 13.0f;
    float TextSizeHeader = 13.0f;
    float TextSizeCaption = 11.0f;
    float TextSizeSmall = 11.0f;
    float TextSizeWindow = 12.0f;

    float BorderWidth = 1.0f;

    float PanelHeaderHeight = 28.0f;
    float ToolbarHeight = 36.0f;
    float SearchBoxHeight = 24.0f;
    float IconButtonSize = 24.0f;
    float ButtonHeight = 24.0f;
    float IconSizeSearch = 14.0f;
    float IconSizeToolbar = 18.0f;
    float IconSizeTree = 14.0f;

    float Space1 = 4.0f;
    float Space2 = 8.0f;
    float Space3 = 12.0f;
    float Space4 = 16.0f;
    float Space5 = 20.0f;
    float Space6 = 24.0f;

    Color TextForState(bool hovered, bool active = false) const;
    Color IconForState(bool hovered, bool active = false) const;
    Color InteractiveBackground(float hoverAnim, float pressAnim, bool selected = false) const;

    static const Theme& Get();
};

} // namespace WindEffects::Editor::UI
