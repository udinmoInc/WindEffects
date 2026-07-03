#pragma once

#include "Core/Geometry.hpp"

namespace we::UI::DesignTokens {

// ────────────────────────────────────────
// TOOLBAR DIMENSIONS
// ────────────────────────────────────────

constexpr float kToolbarHeight = 40.0f;
constexpr float kToolbarPaddingHorizontal = 12.0f;
constexpr float kToolbarPaddingVertical = 4.0f;

// ────────────────────────────────────────
// BUTTON DIMENSIONS
// ────────────────────────────────────────

constexpr float kButtonHeight = 32.0f;
constexpr float kButtonPaddingHorizontal = 14.0f;
constexpr float kButtonSpacing = 8.0f;
constexpr float kButtonGroupSpacing = 16.0f;

// ────────────────────────────────────────
// CORNER RADIUS
// ────────────────────────────────────────

constexpr float kButtonCornerRadius = 6.0f;
constexpr float kIconButtonRadius = 16.0f; // For 32px circular buttons

// ────────────────────────────────────────
// ICON SIZES
// ────────────────────────────────────────

constexpr float kIconSize = 14.0f;
constexpr float kIconButtonSize = 32.0f;
constexpr float kNavigationButtonSize = 28.0f;
constexpr float kNavigationIconSize = 16.0f;

// ────────────────────────────────────────
// DEPTH & ELEVATION
// ────────────────────────────────────────

constexpr float kButtonDepth = 1.0f; // Pixels of physical depth
constexpr float kPressOffset = 1.0f; // Pixels button moves down when pressed

// ────────────────────────────────────────
// BORDERS
// ────────────────────────────────────────

constexpr float kThinBorder = 1.0f;
constexpr float kMediumBorder = 1.5f;

// ────────────────────────────────────────
// ANIMATION DURATION
// ────────────────────────────────────────

constexpr float kHoverAnimationDuration = 0.12f; // 120ms
constexpr float kPressAnimationDuration = 0.08f; // 80ms
constexpr float kHoverDamping = 15.0f;
constexpr float kPressDamping = 25.0f;

// ────────────────────────────────────────
// COLORS - DARK CHARCOAL THEME (INTEGRATED)
// ────────────────────────────────────────

// Primary Button Colors (Create) - Subtle integrated depth
inline Color PrimaryButtonNormal() { return Color{ 0.20f, 0.20f, 0.21f, 1.0f }; } // #333336
inline Color PrimaryButtonHover() { return Color{ 0.24f, 0.24f, 0.25f, 1.0f }; } // #3D3D40
inline Color PrimaryButtonPressed() { return Color{ 0.18f, 0.18f, 0.19f, 1.0f }; } // #2E2E30
inline Color PrimaryButtonBorder() { return Color{ 0.32f, 0.32f, 0.34f, 1.0f }; } // #525256
inline Color PrimaryButtonHighlight() { return Color{ 1.0f, 1.0f, 1.0f, 0.04f }; }
inline Color PrimaryButtonShadow() { return Color{ 0.0f, 0.0f, 0.0f, 0.12f }; }
inline Color PrimaryButtonText() { return Color{ 0.92f, 0.92f, 0.92f, 1.0f }; } // #EBEBEB

// Secondary Button Colors (Import) - Flat integrated
inline Color SecondaryButtonNormal() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; } // Transparent
inline Color SecondaryButtonHover() { return Color{ 1.0f, 1.0f, 1.0f, 0.05f }; } // Subtle hover
inline Color SecondaryButtonPressed() { return Color{ 1.0f, 1.0f, 1.0f, 0.08f }; }
inline Color SecondaryButtonBorder() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; } // No border by default
inline Color SecondaryButtonBorderHover() { return Color{ 0.35f, 0.35f, 0.37f, 1.0f }; } // Border on hover
inline Color SecondaryButtonHighlight() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; }
inline Color SecondaryButtonShadow() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; }
inline Color SecondaryButtonText() { return Color{ 0.82f, 0.82f, 0.82f, 1.0f }; } // #D1D1D1
inline Color SecondaryButtonTextHover() { return Color{ 0.90f, 0.90f, 0.90f, 1.0f }; }

// Icon Button Colors - Recessed integrated
inline Color IconButtonNormal() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; } // Transparent
inline Color IconButtonHover() { return Color{ 1.0f, 1.0f, 1.0f, 0.06f }; }
inline Color IconButtonPressed() { return Color{ 1.0f, 1.0f, 1.0f, 0.10f }; }
inline Color IconButtonBorder() { return Color{ 0.30f, 0.30f, 0.32f, 1.0f }; } // Subtle border
inline Color IconButtonBorderHover() { return Color{ 0.40f, 0.40f, 0.42f, 1.0f }; }
inline Color IconButtonHighlight() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; }
inline Color IconButtonShadow() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; }
inline Color IconButtonIcon() { return Color{ 0.70f, 0.70f, 0.70f, 1.0f }; }
inline Color IconButtonIconHover() { return Color{ 0.85f, 0.85f, 0.85f, 1.0f }; }

// Navigation Button Colors - Minimal integrated
inline Color NavigationButtonNormal() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; } // Transparent
inline Color NavigationButtonHover() { return Color{ 1.0f, 1.0f, 1.0f, 0.05f }; } // Very subtle hover
inline Color NavigationButtonPressed() { return Color{ 1.0f, 1.0f, 1.0f, 0.08f }; }
inline Color NavigationButtonBorder() { return Color{ 0.0f, 0.0f, 0.0f, 0.0f }; } // No border by default
inline Color NavigationButtonBorderHover() { return Color{ 0.35f, 0.35f, 0.37f, 1.0f }; } // Thin border on hover
inline Color NavigationButtonIcon() { return Color{ 0.65f, 0.65f, 0.65f, 1.0f }; } // 70% opacity
inline Color NavigationButtonIconHover() { return Color{ 0.90f, 0.90f, 0.90f, 1.0f }; } // 100% on hover

// Disabled State
inline Color DisabledBackground() { return Color{ 0.14f, 0.14f, 0.15f, 1.0f }; }
inline Color DisabledBorder() { return Color{ 0.20f, 0.20f, 0.22f, 1.0f }; }
inline Color DisabledText() { return Color{ 0.45f, 0.45f, 0.45f, 1.0f }; }
inline Color DisabledIcon() { return Color{ 0.40f, 0.40f, 0.40f, 1.0f }; }

// Focused State
inline Color FocusedOutline() { return Color{ 0.50f, 0.50f, 0.55f, 1.0f }; }

// ────────────────────────────────────────
// BREADCRUMB COLORS
// ────────────────────────────────────────

inline Color BreadcrumbCurrent() { return Color{ 0.90f, 0.90f, 0.90f, 1.0f }; }
inline Color BreadcrumbPrevious() { return Color{ 0.60f, 0.60f, 0.60f, 1.0f }; }
inline Color BreadcrumbHover() { return Color{ 1.0f, 1.0f, 1.0f, 1.0f }; }
inline Color BreadcrumbSeparator() { return Color{ 0.45f, 0.45f, 0.45f, 1.0f }; }
inline Color BreadcrumbHoverBg() { return Color{ 1.0f, 1.0f, 1.0f, 0.06f }; }

// ────────────────────────────────────────
// TOOLBAR DEPTH
// ────────────────────────────────────────

inline Color ToolbarTopHighlight() { return Color{ 1.0f, 1.0f, 1.0f, 0.03f }; }
inline Color ToolbarBottomShadow() { return Color{ 0.0f, 0.0f, 0.0f, 0.12f }; }
inline Color ToolbarSeparator() { return Color{ 0.0f, 0.0f, 0.0f, 0.20f }; }

} // namespace we::UI::DesignTokens
