#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Styles/StyleProperty.h"
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

// Fully resolved visual appearance produced by the style pipeline.
struct ResolvedVisualStyle {
    Color background = Color::Transparent();
    Color foreground = Color::White();
    Color border = Color::Transparent();
    Color icon = Color::White();
    float cornerRadius = 0.0f;
    float borderWidth = 0.0f;
    float fontSize = 13.0f;
    Margin padding{};
    float height = 0.0f;
    float iconSize = 16.0f;
    bool bold = false;
    int elevation = 0;
    float opacity = 1.0f;
    float scale = 1.0f;
    Point translation{};
    float rotation = 0.0f;
    ShadowStyle shadow{};
};

struct ButtonStyle {
    ColorProperty background{ColorToken::ControlBackground};
    ColorProperty backgroundHover{ColorToken::ControlBackgroundHover};
    ColorProperty backgroundPressed{ColorToken::ControlBackgroundPressed};
    ColorProperty backgroundDisabled{ColorToken::ControlBackgroundDisabled};
    ColorProperty foreground{ColorToken::TextPrimary};
    ColorProperty foregroundDisabled{ColorToken::TextDisabled};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Small};
    PaddingStyle padding{};
    TypographyStyle typography{};
    ElevationProperty elevation{ElevationToken::None};
    AnimationProperty transition{AnimationToken::Fast};
    SpacingProperty minHeight{SpacingToken::Large};
};

struct LabelStyle {
    TypographyStyle typography{};
    ColorProperty foreground{ColorToken::TextPrimary};
    ColorProperty foregroundSecondary{ColorToken::TextSecondary};
    ColorProperty foregroundDisabled{ColorToken::TextDisabled};
    PaddingStyle padding{};
};

struct WindowStyle {
    ColorProperty background{ColorToken::WindowBackground};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Medium};
    ElevationProperty elevation{ElevationToken::Window};
    PaddingStyle padding{};
    TypographyStyle titleTypography{TypographyToken::Title};
};

struct MenuStyle {
    ColorProperty background{ColorToken::SecondarySurface};
    ColorProperty itemBackground{ColorToken::ControlBackground};
    ColorProperty itemBackgroundHover{ColorToken::ControlBackgroundHover};
    ColorProperty itemBackgroundSelected{ColorToken::ControlBackgroundSelected};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Medium};
    ElevationProperty elevation{ElevationToken::Popup};
    PaddingStyle padding{};
    TypographyStyle typography{};
    AnimationProperty transition{AnimationToken::Fast};
};

struct TreeViewStyle {
    ColorProperty background{ColorToken::PrimarySurface};
    ColorProperty rowBackground{ColorToken::ControlBackground};
    ColorProperty rowBackgroundHover{ColorToken::ControlBackgroundHover};
    ColorProperty rowBackgroundSelected{ColorToken::ControlBackgroundSelected};
    ColorProperty foreground{ColorToken::TextPrimary};
    ColorProperty foregroundSecondary{ColorToken::TextSecondary};
    BorderStyleTokens border{};
    PaddingStyle padding{};
    SpacingProperty rowHeight{SpacingToken::Large};
    TypographyStyle typography{};
};

struct TableViewStyle {
    ColorProperty background{ColorToken::PrimarySurface};
    ColorProperty headerBackground{ColorToken::SecondarySurface};
    ColorProperty rowBackground{ColorToken::ControlBackground};
    ColorProperty rowBackgroundHover{ColorToken::ControlBackgroundHover};
    ColorProperty rowBackgroundSelected{ColorToken::ControlBackgroundSelected};
    ColorProperty foreground{ColorToken::TextPrimary};
    ColorProperty foregroundSecondary{ColorToken::TextSecondary};
    BorderStyleTokens border{};
    PaddingStyle cellPadding{};
    SpacingProperty rowHeight{SpacingToken::Large};
    TypographyStyle typography{};
};

struct GridStyle {
    ColorProperty background{ColorToken::PrimarySurface};
    ColorProperty cellBackground{ColorToken::ControlBackground};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    SpacingProperty gap{SpacingToken::Medium};
    PaddingStyle padding{};
};

struct FlexStyle {
    ColorProperty background{ColorToken::PrimarySurface};
    SpacingProperty gap{SpacingToken::Medium};
    PaddingStyle padding{};
};

struct SearchBoxStyle {
    ColorProperty background{ColorToken::ControlBackground};
    ColorProperty backgroundFocused{ColorToken::ControlBackgroundSelected};
    ColorProperty foreground{ColorToken::TextPrimary};
    ColorProperty placeholder{ColorToken::TextSecondary};
    BorderStyleTokens border{};
    BorderStyleTokens borderFocused{};
    RadiusProperty cornerRadius{RadiusToken::Small};
    PaddingStyle padding{};
    TypographyStyle typography{};
    SpacingProperty height{SpacingToken::Large};
    AnimationProperty transition{AnimationToken::Fast};
};

struct TextBoxStyle {
    ColorProperty background{ColorToken::ControlBackground};
    ColorProperty backgroundFocused{ColorToken::ControlBackgroundSelected};
    ColorProperty foreground{ColorToken::TextPrimary};
    ColorProperty foregroundDisabled{ColorToken::TextDisabled};
    BorderStyleTokens border{};
    BorderStyleTokens borderFocused{};
    BorderStyleTokens borderError{};
    RadiusProperty cornerRadius{RadiusToken::Small};
    PaddingStyle padding{};
    TypographyStyle typography{};
    SpacingProperty height{SpacingToken::Large};
    AnimationProperty transition{AnimationToken::Fast};
};

struct CheckBoxStyle {
    ColorProperty boxBackground{ColorToken::ControlBackground};
    ColorProperty boxBackgroundChecked{ColorToken::AccentSurface};
    ColorProperty checkmark{ColorToken::TextOnAccent};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Small};
    SpacingProperty boxSize{SpacingToken::Medium};
    PaddingStyle padding{};
    TypographyStyle typography{};
    AnimationProperty transition{AnimationToken::Fast};
};

struct RadioButtonStyle {
    ColorProperty circleBackground{ColorToken::ControlBackground};
    ColorProperty circleBackgroundSelected{ColorToken::AccentSurface};
    ColorProperty dot{ColorToken::TextOnAccent};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    SpacingProperty circleSize{SpacingToken::Medium};
    PaddingStyle padding{};
    TypographyStyle typography{};
    AnimationProperty transition{AnimationToken::Fast};
};

struct ComboBoxStyle {
    ColorProperty background{ColorToken::ControlBackground};
    ColorProperty backgroundHover{ColorToken::ControlBackgroundHover};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Small};
    PaddingStyle padding{};
    TypographyStyle typography{};
    ElevationProperty elevation{ElevationToken::Card};
    AnimationProperty transition{AnimationToken::Fast};
};

struct DialogStyle {
    ColorProperty background{ColorToken::SecondarySurface};
    ColorProperty scrim{ColorToken::ScrimOverlay};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Large};
    ElevationProperty elevation{ElevationToken::Overlay};
    PaddingStyle padding{};
    TypographyStyle titleTypography{TypographyToken::Title};
    TypographyStyle bodyTypography{TypographyToken::Body};
    AnimationProperty transition{AnimationToken::Normal};
};

struct TooltipStyle {
    ColorProperty background{ColorToken::SecondarySurface};
    ColorProperty foreground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    RadiusProperty cornerRadius{RadiusToken::Small};
    ElevationProperty elevation{ElevationToken::Popup};
    PaddingStyle padding{};
    TypographyStyle typography{TypographyToken::Caption};
    AnimationProperty transition{AnimationToken::Fast};
};

struct ScrollBarStyle {
    ColorProperty track{ColorToken::TertiarySurface};
    ColorProperty thumb{ColorToken::BorderDefault};
    ColorProperty thumbHover{ColorToken::BorderFocused};
    RadiusProperty cornerRadius{RadiusToken::Full};
    SpacingProperty width{SpacingToken::Small};
    AnimationProperty transition{AnimationToken::Fast};
};

struct ProgressBarStyle {
    ColorProperty track{ColorToken::TertiarySurface};
    ColorProperty fill{ColorToken::AccentSurface};
    ColorProperty fillSuccess{ColorToken::SuccessColor};
    ColorProperty fillWarning{ColorToken::WarningColor};
    ColorProperty fillError{ColorToken::ErrorColor};
    RadiusProperty cornerRadius{RadiusToken::Full};
    SpacingProperty height{SpacingToken::Small};
    AnimationProperty transition{AnimationToken::Normal};
};

struct PropertyGridStyle {
    ColorProperty background{ColorToken::PrimarySurface};
    ColorProperty rowBackground{ColorToken::ControlBackground};
    ColorProperty rowBackgroundHover{ColorToken::ControlBackgroundHover};
    ColorProperty rowBackgroundSelected{ColorToken::ControlBackgroundSelected};
    ColorProperty labelForeground{ColorToken::TextSecondary};
    ColorProperty valueForeground{ColorToken::TextPrimary};
    BorderStyleTokens border{};
    PaddingStyle padding{};
    SpacingProperty rowHeight{SpacingToken::Large};
    TypographyStyle labelTypography{TypographyToken::Caption};
    TypographyStyle valueTypography{TypographyToken::Body};
};

} // namespace we::runtime::kindui
