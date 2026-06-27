#pragma once

#include "Geometry.hpp"
#include <string>
#include <memory>

namespace HouseEngine::UI {

struct Theme {
    // ------------------------------------------------------------------------
    // Colors (AAA Neutral Dark Theme - Industrial 2026 Style)
    // ------------------------------------------------------------------------
    // Core UI Colors
    Color WindowBackground  = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color WorkspaceBackground = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color ToolbarBackground = { 0.165f, 0.165f, 0.165f, 1.0f }; // #2A2A2A
    Color PanelBackground   = { 0.145f, 0.145f, 0.145f, 1.0f }; // #252525
    Color HeaderBackground  = { 0.145f, 0.145f, 0.145f, 1.0f }; // #252525 (MenuBar/TitleBar)
    Color ViewportBackground = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030 (Viewport Toolbar)
    Color MenuBarBackground = { 0.145f, 0.145f, 0.145f, 1.0f }; // #252525
    Color TabBackground     = { 0.169f, 0.169f, 0.169f, 1.0f }; // #2B2B2B
    Color PopupBackground   = { 0.149f, 0.149f, 0.149f, 1.0f }; // #262626
    
    // Borders
    Color BorderDefault     = { 0.204f, 0.204f, 0.204f, 1.0f }; // #343434
    Color BorderLight       = { 0.204f, 0.204f, 0.204f, 1.0f }; // #343434
    Color BorderDark        = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E
    Color Separator         = { 1.0f, 1.0f, 1.0f, 0.12f }; // 12% opacity (10-15%)

    // Interactive States
    Color HoverOverlay      = { 0.196f, 0.196f, 0.196f, 1.0f }; // #323232
    Color PressedOverlay    = { 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A
    Color SelectedBg        = { 0.231f, 0.510f, 0.965f, 1.0f }; // #3B82F6

    // Text & Content
    Color TextPrimary       = { 0.847f, 0.847f, 0.847f, 1.0f }; // #D8D8D8
    Color TextSecondary     = { 0.659f, 0.659f, 0.659f, 1.0f }; // #A8A8A8
    Color TextDisabled      = { 0.400f, 0.400f, 0.400f, 1.0f }; // #666666
    Color TextWindowLabel   = { 0.659f, 0.659f, 0.659f, 1.0f }; // #A8A8A8

    // Accents
    Color SelectedAccent    = { 0.231f, 0.510f, 0.965f, 1.0f }; // #3B82F6
    Color ActiveTabLine     = { 0.231f, 0.510f, 0.965f, 1.0f }; // #3B82F6
    Color Success           = { 0.180f, 0.800f, 0.443f, 1.0f };
    Color Warning           = { 0.945f, 0.768f, 0.058f, 1.0f };

    // Input Fields
    Color InputBackground   = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color SearchBoxBg       = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color SearchPlaceholder = { 0.467f, 0.467f, 0.467f, 1.0f }; // #777777

    // ------------------------------------------------------------------------
    // Geometry & Layout Constants
    // ------------------------------------------------------------------------
    float CornerRadiusSmall = 2.0f;
    float CornerRadiusMedium= 4.0f;
    float CornerRadiusLarge = 4.0f; 
    
    // Typography
    float TextSizeSection   = 15.0f; // Section titles
    float TextSizeTabs      = 14.0f; // Tabs
    float TextSizeMenu      = 13.0f; // Menu
    float TextSizeWindow    = 13.0f; // Window Title
    float TextSizeProperty  = 12.0f; // Properties
    float TextSizeCaption   = 11.0f; // Captions
    
    // Defaults aliases
    float TextSizeHeader    = 15.0f;
    float TextSizeLarge     = 14.0f;
    float TextSizeNormal    = 13.0f;
    float TextSizeToolbar   = 13.0f;
    float TextSizeBody      = 12.0f; 
    float TextSizeSmall     = 11.0f; 
    
    float BorderWidth       = 1.0f;
    
    // Core metrics (8px grid)
    Margin PaddingWindow    = { 0.0f, 0.0f, 0.0f, 0.0f };
    Margin PaddingPanel     = { 2.0f, 2.0f, 2.0f, 2.0f }; // tighter panels
    Margin PaddingButton    = { 6.0f, 2.0f, 6.0f, 2.0f }; // tighter buttons
    Margin PaddingIconBtn   = { 4.0f, 4.0f, 4.0f, 4.0f };

    // ------------------------------------------------------------------------
    // Global Management
    // ------------------------------------------------------------------------
    static Theme& Get();
};

} // namespace HouseEngine::UI
