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
    Color WindowBackground  = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E
    Color WorkspaceBackground = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E
    Color ToolbarBackground = { 0.176f, 0.176f, 0.188f, 1.0f }; // #2D2D30
    Color PanelBackground   = { 0.145f, 0.145f, 0.149f, 1.0f }; // #252526
    Color HeaderBackground  = { 0.145f, 0.145f, 0.149f, 1.0f }; // #252526
    Color ViewportBackground = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E
    Color MenuBarBackground = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E
    Color TabBackground     = { 0.145f, 0.145f, 0.149f, 1.0f }; // #252526
    Color PopupBackground   = { 0.145f, 0.145f, 0.149f, 1.0f }; // #252526
    
    // Borders (rgba(255,255,255,0.06))
    Color BorderDefault     = { 1.0f, 1.0f, 1.0f, 0.06f };
    Color BorderLight       = { 1.0f, 1.0f, 1.0f, 0.06f };
    Color BorderDark        = { 1.0f, 1.0f, 1.0f, 0.06f };
    Color Separator         = { 1.0f, 1.0f, 1.0f, 0.06f };

    // Interactive States
    Color HoverOverlay      = { 0.200f, 0.200f, 0.216f, 1.0f }; // #333337
    Color PressedOverlay    = { 0.250f, 0.250f, 0.260f, 1.0f }; // Darker/Lighter for press
    Color SelectedBg        = { 0.227f, 0.478f, 0.996f, 0.3f }; // #3A7AFE (Translucent for bg)

    // Text & Content
    Color TextPrimary       = { 0.941f, 0.941f, 0.941f, 1.0f }; // #F0F0F0
    Color TextSecondary     = { 0.604f, 0.604f, 0.604f, 1.0f }; // #9A9A9A
    Color TextDisabled      = { 0.400f, 0.400f, 0.400f, 1.0f }; // #666666

    // Accents
    Color SelectedAccent    = { 0.227f, 0.478f, 0.996f, 1.0f }; // #3A7AFE (Accent Blue)
    Color ActiveTabLine     = { 0.227f, 0.478f, 0.996f, 1.0f }; // #3A7AFE
    Color Success           = { 0.180f, 0.800f, 0.443f, 1.0f };
    Color Warning           = { 0.945f, 0.768f, 0.058f, 1.0f };

    // Input Fields
    Color InputBackground   = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E
    Color SearchBoxBg       = { 0.118f, 0.118f, 0.118f, 1.0f }; // #1E1E1E

    // ------------------------------------------------------------------------
    // Geometry & Layout Constants
    // ------------------------------------------------------------------------
    float CornerRadiusSmall = 2.0f;
    float CornerRadiusMedium= 4.0f;
    float CornerRadiusLarge = 4.0f; 
    
    // Typography
    float TextSizeHeader    = 15.0f; // Section title
    float TextSizeLarge     = 14.0f;
    float TextSizeNormal    = 13.0f; // Default
    float TextSizeToolbar   = 13.0f;
    float TextSizeBody      = 13.0f; 
    float TextSizeMenu      = 13.0f;
    float TextSizeSmall     = 11.0f; // Captions
    
    float BorderWidth       = 1.0f;
    
    // Core metrics (8px grid)
    Margin PaddingWindow    = { 0.0f, 0.0f, 0.0f, 0.0f }; // Viewport dominates, no window border padding
    Margin PaddingPanel     = { 4.0f, 4.0f, 4.0f, 4.0f }; // Minimal padding
    Margin PaddingButton    = { 8.0f, 4.0f, 8.0f, 4.0f }; // Clickable areas
    Margin PaddingIconBtn   = { 4.0f, 4.0f, 4.0f, 4.0f };

    // ------------------------------------------------------------------------
    // Global Management
    // ------------------------------------------------------------------------
    static Theme& Get() {
        static Theme instance;
        return instance;
    }
};

} // namespace HouseEngine::UI
