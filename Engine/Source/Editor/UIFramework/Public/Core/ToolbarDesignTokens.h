#pragma once

#include "Core/Theme.h"

namespace WindEffects::Editor::UI {
namespace DesignTokens {

constexpr float kNavigationButtonSize = 28.0f;
constexpr float kIconButtonSize = 28.0f;
constexpr float kIconButtonRadius = 4.0f;
constexpr float kButtonHeight = 28.0f;
constexpr float kButtonPaddingHorizontal = 10.0f;
constexpr float kButtonSpacing = 4.0f;
constexpr float kButtonGroupSpacing = 12.0f;
constexpr float kButtonCornerRadius = 4.0f;
constexpr float kNavigationIconSize = 16.0f;
constexpr float kIconSize = 18.0f;
constexpr float kHoverDamping = 12.0f;
constexpr float kPressDamping = 12.0f;
constexpr float kPressOffset = 1.0f;

inline Color NavigationButtonNormal() { return Color::Transparent(); }
inline Color NavigationButtonBorder() { return Theme::Get().BorderDefault; }
inline Color NavigationButtonIcon() { return Theme::Get().IconDefault; }
inline Color NavigationButtonHover() { return Theme::Get().HoverBackground; }
inline Color NavigationButtonIconHover() { return Theme::Get().IconHover; }
inline Color NavigationButtonBorderHover() { return Theme::Get().BorderLight; }
inline Color NavigationButtonPressed() { return Theme::Get().PressedBackground; }
inline Color DisabledBackground() { return Theme::Get().DisabledBackground; }
inline Color DisabledBorder() { return Theme::Get().BorderDark; }
inline Color DisabledIcon() { return Theme::Get().IconDisabled; }
inline Color DisabledText() { return Theme::Get().TextDisabled; }

inline Color PrimaryButtonNormal() { return Theme::Get().ButtonPrimaryBackground; }
inline Color PrimaryButtonBorder() { return Theme::Get().BorderDefault; }
inline Color PrimaryButtonText() { return Theme::Get().TextPrimary; }
inline Color PrimaryButtonHover() { return Theme::Get().ButtonPrimaryHover; }
inline Color PrimaryButtonPressed() { return Theme::Get().ButtonPrimaryPressed; }
inline Color PrimaryButtonHighlight() { return Theme::Get().BorderLight; }

inline Color SecondaryButtonNormal() { return Color::Transparent(); }
inline Color SecondaryButtonBorder() { return Theme::Get().BorderDefault; }
inline Color SecondaryButtonText() { return Theme::Get().TextSecondary; }
inline Color SecondaryButtonHover() { return Theme::Get().HoverBackground; }
inline Color SecondaryButtonTextHover() { return Theme::Get().TextPrimary; }
inline Color SecondaryButtonBorderHover() { return Theme::Get().BorderLight; }
inline Color SecondaryButtonPressed() { return Theme::Get().PressedBackground; }

inline Color IconButtonNormal() { return Color::Transparent(); }
inline Color IconButtonBorder() { return Theme::Get().BorderDefault; }
inline Color IconButtonIcon() { return Theme::Get().IconDefault; }
inline Color IconButtonHover() { return Theme::Get().HoverBackground; }
inline Color IconButtonIconHover() { return Theme::Get().IconHover; }
inline Color IconButtonBorderHover() { return Theme::Get().BorderLight; }
inline Color IconButtonPressed() { return Theme::Get().PressedBackground; }

inline Color ToolbarTopHighlight() { return Theme::Get().BorderLight; }
inline Color ToolbarSeparator() { return Theme::Get().Separator; }
inline Color ToolbarBottomShadow() { return Theme::Get().BorderDark; }

} // namespace DesignTokens
} // namespace WindEffects::Editor::UI
