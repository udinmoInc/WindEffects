#include "KindUI/Tokens/SurfaceRole.h"

#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Profiling/UiColorDebug.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>

namespace we::runtime::kindui {
namespace {

ColorToken TokenForRole(SurfaceRole role) {
    switch (role) {
    case SurfaceRole::Window:
        return ColorToken::WindowBackground;
    case SurfaceRole::Toolbar:
        return ColorToken::ToolbarBackground;
    case SurfaceRole::DockChrome:
        return ColorToken::DockChromeBackground;
    case SurfaceRole::TabActive:
        return ColorToken::TabActiveBackground;
    case SurfaceRole::TabInactive:
        return ColorToken::TabBackground;
    case SurfaceRole::Panel:
        return ColorToken::PanelBackground;
    case SurfaceRole::PanelHeader:
        return ColorToken::HeaderBackground;
    case SurfaceRole::Recessed:
        return ColorToken::SecondarySurface;
    case SurfaceRole::Input:
        return ColorToken::InputBackground;
    case SurfaceRole::InputBorder:
        return ColorToken::BorderDefault;
    case SurfaceRole::Control:
        return ColorToken::CardBackground;
    case SurfaceRole::ControlHover:
        return ColorToken::HoverBackground;
    case SurfaceRole::ControlPressed:
        return ColorToken::PressedBackground;
    case SurfaceRole::Selected:
        return ColorToken::SelectedBackground;
    case SurfaceRole::SelectedInactive:
        return ColorToken::SelectInactiveBackground;
    case SurfaceRole::Text:
        return ColorToken::TextPrimary;
    case SurfaceRole::TextSecondary:
        return ColorToken::TextSecondary;
    case SurfaceRole::TextHint:
        return ColorToken::TextHint;
    case SurfaceRole::TextDisabled:
        return ColorToken::TextDisabled;
    case SurfaceRole::Separator:
        return ColorToken::Separator;
    case SurfaceRole::Border:
        return ColorToken::BorderDefault;
    case SurfaceRole::StatusBar:
        return ColorToken::WorkspaceBackground;
    case SurfaceRole::ViewportToolbar:
        return ColorToken::ViewportToolbarBackground;
    case SurfaceRole::Accent:
        return ColorToken::AccentPrimary;
    case SurfaceRole::Disabled:
        return ColorToken::DisabledBackground;
    case SurfaceRole::Popup:
        return ColorToken::PopupBackground;
    case SurfaceRole::Transparent:
    case SurfaceRole::None:
    default:
        return ColorToken::PanelBackground;
    }
}

} // namespace

ColorToken SurfaceRoleToColorToken(SurfaceRole role) {
    return TokenForRole(role);
}

SurfaceRole SurfaceRoleFromColorToken(ColorToken token) {
    switch (token) {
    case ColorToken::WindowBackground:
        return SurfaceRole::Window;
    case ColorToken::ToolbarBackground:
        return SurfaceRole::Toolbar;
    case ColorToken::WorkspaceBackground:
    case ColorToken::DockChromeBackground:
        return SurfaceRole::DockChrome;
    case ColorToken::TabActiveBackground:
        return SurfaceRole::TabActive;
    case ColorToken::TabBackground:
        return SurfaceRole::TabInactive;
    case ColorToken::PanelBackground:
        return SurfaceRole::Panel;
    case ColorToken::HeaderBackground:
    case ColorToken::ListLabelBandBackground:
        return SurfaceRole::PanelHeader;
    case ColorToken::SecondarySurface:
        return SurfaceRole::Recessed;
    case ColorToken::InputBackground:
    case ColorToken::ControlBackground:
        return SurfaceRole::Input;
    case ColorToken::BorderDefault:
    case ColorToken::BorderSubtle:
        return SurfaceRole::InputBorder;
    case ColorToken::CardBackground:
    case ColorToken::PopupBackground:
        return SurfaceRole::Control;
    case ColorToken::HoverBackground:
    case ColorToken::ControlBackgroundHover:
        return SurfaceRole::ControlHover;
    case ColorToken::PressedBackground:
    case ColorToken::ControlBackgroundPressed:
        return SurfaceRole::ControlPressed;
    case ColorToken::SelectedBackground:
    case ColorToken::ControlBackgroundSelected:
        return SurfaceRole::Selected;
    case ColorToken::SelectInactiveBackground:
        return SurfaceRole::SelectedInactive;
    case ColorToken::Separator:
        return SurfaceRole::Separator;
    case ColorToken::StatusBarBackground:
        return SurfaceRole::StatusBar;
    case ColorToken::ViewportToolbarBackground:
        return SurfaceRole::ViewportToolbar;
    case ColorToken::AccentPrimary:
        return SurfaceRole::Accent;
    case ColorToken::DisabledBackground:
    case ColorToken::ControlBackgroundDisabled:
        return SurfaceRole::Disabled;
    default:
        return SurfaceRole::Panel;
    }
}

TextRole TextRoleFromSurfaceRole(SurfaceRole role) {
    switch (role) {
    case SurfaceRole::TextSecondary:
        return TextRole::Secondary;
    case SurfaceRole::TextHint:
        return TextRole::Hint;
    case SurfaceRole::TextDisabled:
        return TextRole::Disabled;
    case SurfaceRole::Text:
    default:
        return TextRole::Primary;
    }
}

const char* SurfaceRoleName(SurfaceRole role) {
    switch (role) {
    case SurfaceRole::Window: return "Window";
    case SurfaceRole::Toolbar: return "Toolbar";
    case SurfaceRole::DockChrome: return "DockChrome";
    case SurfaceRole::TabActive: return "TabActive";
    case SurfaceRole::TabInactive: return "TabInactive";
    case SurfaceRole::Panel: return "Panel";
    case SurfaceRole::PanelHeader: return "PanelHeader";
    case SurfaceRole::Recessed: return "Recessed";
    case SurfaceRole::Input: return "Input";
    case SurfaceRole::InputBorder: return "InputBorder";
    case SurfaceRole::Control: return "Control";
    case SurfaceRole::ControlHover: return "ControlHover";
    case SurfaceRole::ControlPressed: return "ControlPressed";
    case SurfaceRole::Selected: return "Selected";
    case SurfaceRole::SelectedInactive: return "SelectedInactive";
    case SurfaceRole::Text: return "Text";
    case SurfaceRole::TextSecondary: return "TextSecondary";
    case SurfaceRole::TextHint: return "TextHint";
    case SurfaceRole::TextDisabled: return "TextDisabled";
    case SurfaceRole::Separator: return "Separator";
    case SurfaceRole::Border: return "Border";
    case SurfaceRole::StatusBar: return "StatusBar";
    case SurfaceRole::ViewportToolbar: return "ViewportToolbar";
    case SurfaceRole::Accent: return "Accent";
    case SurfaceRole::Disabled: return "Disabled";
    case SurfaceRole::Popup: return "Popup";
    case SurfaceRole::Transparent: return "Transparent";
    case SurfaceRole::None:
    default:
        return "None";
    }
}

const char* TextRoleName(TextRole role) {
    switch (role) {
    case TextRole::Primary: return "Text";
    case TextRole::Secondary: return "TextSecondary";
    case TextRole::Hint: return "TextHint";
    case TextRole::Disabled: return "TextDisabled";
    case TextRole::OnAccent: return "TextOnAccent";
    case TextRole::Header: return "TextHeader";
    default:
        return "Text";
    }
}

const char* ControlStateName(ControlState state) {
    switch (state) {
    case ControlState::Normal: return "Normal";
    case ControlState::Hover: return "Hover";
    case ControlState::Pressed: return "Pressed";
    case ControlState::Checked: return "Checked";
    case ControlState::Selected: return "Selected";
    case ControlState::SelectedInactive: return "SelectedInactive";
    case ControlState::Disabled: return "Disabled";
    case ControlState::Focused: return "Focused";
    default:
        return "Normal";
    }
}

Color ResolveSurfaceColor(SurfaceRole role) {
    if (role == SurfaceRole::Transparent || role == SurfaceRole::None) {
        return Color::Transparent();
    }
    const Color resolved = ResolveColor(TokenForRole(role));
    if (UiColorDebug::IsSemanticAuditEnabled()) {
        UiColorDebug::Get().TraceResolveSurface(role, resolved);
    }
    return resolved;
}

Color ResolveTextColor(TextRole role) {
    switch (role) {
    case TextRole::Primary:
        return ResolveColor(ColorToken::TextPrimary);
    case TextRole::Secondary:
        return ResolveColor(ColorToken::TextSecondary);
    case TextRole::Hint:
        return ResolveColor(ColorToken::TextHint);
    case TextRole::Disabled:
        return ResolveColor(ColorToken::TextDisabled);
    case TextRole::OnAccent:
        return ResolveColor(ColorToken::TextOnAccent);
    case TextRole::Header:
        return ResolveColor(ColorToken::IconPrimary);
    default:
        return ResolveColor(ColorToken::TextPrimary);
    }
}

Color ResolveControlColor(ControlKind kind, ControlState state) {
    switch (kind) {
    case ControlKind::Button:
        switch (state) {
        case ControlState::Disabled:
            return ResolveSurfaceColor(SurfaceRole::Disabled);
        case ControlState::Pressed:
            return ResolveSurfaceColor(SurfaceRole::ControlPressed);
        case ControlState::Hover:
        case ControlState::Focused:
            return ResolveSurfaceColor(SurfaceRole::ControlHover);
        case ControlState::Checked:
        case ControlState::Selected:
            return ResolveSurfaceColor(SurfaceRole::Selected);
        case ControlState::SelectedInactive:
            return ResolveSurfaceColor(SurfaceRole::SelectedInactive);
        case ControlState::Normal:
        default:
            return ResolveSurfaceColor(SurfaceRole::Control);
        }

    case ControlKind::TreeRow:
        switch (state) {
        case ControlState::Selected:
            return ResolveSurfaceColor(SurfaceRole::Selected);
        case ControlState::SelectedInactive:
            return ResolveSurfaceColor(SurfaceRole::SelectedInactive);
        case ControlState::Hover:
        case ControlState::Pressed:
        case ControlState::Focused:
            return ResolveSurfaceColor(SurfaceRole::ControlHover);
        case ControlState::Disabled:
            return ResolveSurfaceColor(SurfaceRole::Disabled);
        case ControlState::Normal:
        case ControlState::Checked:
        default:
            return Color::Transparent();
        }

    case ControlKind::Tab:
        switch (state) {
        case ControlState::Selected:
        case ControlState::Checked:
            return ResolveSurfaceColor(SurfaceRole::TabActive);
        case ControlState::Hover:
        case ControlState::Focused:
            return ResolveSurfaceColor(SurfaceRole::ControlHover);
        case ControlState::Disabled:
            return ResolveSurfaceColor(SurfaceRole::Disabled);
        case ControlState::Pressed:
            return ResolveSurfaceColor(SurfaceRole::ControlPressed);
        case ControlState::SelectedInactive:
            return ResolveSurfaceColor(SurfaceRole::TabInactive);
        case ControlState::Normal:
        default:
            return ResolveSurfaceColor(SurfaceRole::TabInactive);
        }

    case ControlKind::Input:
        switch (state) {
        case ControlState::Disabled:
            return ResolveSurfaceColor(SurfaceRole::Disabled);
        case ControlState::Focused:
            return ResolveSurfaceColor(SurfaceRole::Input);
        case ControlState::Hover:
            return ResolveSurfaceColor(SurfaceRole::ControlHover);
        case ControlState::Pressed:
            return ResolveSurfaceColor(SurfaceRole::ControlPressed);
        case ControlState::Normal:
        default:
            return ResolveSurfaceColor(SurfaceRole::Input);
        }

    case ControlKind::Generic:
    default:
        switch (state) {
        case ControlState::Disabled:
            return ResolveSurfaceColor(SurfaceRole::Disabled);
        case ControlState::Pressed:
            return ResolveSurfaceColor(SurfaceRole::ControlPressed);
        case ControlState::Hover:
        case ControlState::Focused:
            return ResolveSurfaceColor(SurfaceRole::ControlHover);
        case ControlState::Selected:
            return ResolveSurfaceColor(SurfaceRole::Selected);
        case ControlState::SelectedInactive:
            return ResolveSurfaceColor(SurfaceRole::SelectedInactive);
        case ControlState::Checked:
        case ControlState::Normal:
        default:
            return ResolveSurfaceColor(SurfaceRole::Control);
        }
    }
}

Color ResolveInteractiveSurfaceColor(
    SurfaceRole baseRole,
    float hoverAnim,
    float pressAnim,
    bool selected)
{
    if (baseRole == SurfaceRole::Transparent || baseRole == SurfaceRole::None) {
        return Color::Transparent();
    }
    return ResolveInteractiveBackground(
        hoverAnim,
        pressAnim,
        selected,
        TokenForRole(baseRole));
}

} // namespace we::runtime::kindui
