#pragma once

#include "Application/Export.hpp"

#include "Core/Geometry.hpp"
#include <string>
#include <memory>

namespace we::UI {

struct APPLICATION_API Theme {
    // ------------------------------------------------------------------------
    // Colors (AAA Neutral Dark Theme - Layered 2026 Style)
    // ------------------------------------------------------------------------
    // Core UI Colors
    // Core UI Colors
    Color WindowBackground       = { 0.090f, 0.090f, 0.090f, 1.0f }; // #171717
    Color WorkspaceBackground    = { 0.114f, 0.114f, 0.114f, 1.0f }; // #1D1D1D
    Color ToolbarBackground      = { 0.114f, 0.114f, 0.114f, 1.0f }; // #1D1D1D
    Color PanelBackground        = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color HeaderBackground       = { 0.125f, 0.125f, 0.125f, 1.0f }; // #202020
    Color ViewportBackground     = { 0.090f, 0.090f, 0.090f, 1.0f }; // #171717
    Color FooterBackground       = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color MenuBarBackground      = { 0.125f, 0.125f, 0.125f, 1.0f }; // #202020
    Color TabBackground          = { 0.114f, 0.114f, 0.114f, 1.0f }; // #1D1D1D
    Color PopupBackground        = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color ContentBrowserBackground = { 0.137f, 0.137f, 0.137f, 1.0f }; // #232323
    Color ContentBrowserFolderIcon   = { 0.647f, 0.671f, 0.710f, 1.0f }; // #A5ABB5
    Color ContentBrowserItemLabel    = { 0.827f, 0.839f, 0.859f, 1.0f }; // #D3D6DB
    Color ContentBrowserHoverBg      = { 0.157f, 0.157f, 0.161f, 1.0f }; // subtle lift on hover
    Color ContentBrowserSelectedFill = { 0.0f, 0.0f, 0.0f, 0.22f };      // subtle dark selection fill

    // Content Browser folder thumbnail (filled artwork â€” not Lucide)
    Color ContentBrowserFolderBody      = { 0.553f, 0.424f, 0.235f, 1.0f }; // darker body (~#8D6B3C)
    Color ContentBrowserFolderTab       = { 0.702f, 0.541f, 0.298f, 1.0f }; // tab flap (~#B38A4C)
    Color ContentBrowserFolderShadow    = { 0.431f, 0.329f, 0.180f, 1.0f }; // depth shadow (~#6E542E)
    Color ContentBrowserFolderHighlight = { 0.788f, 0.647f, 0.380f, 1.0f }; // lighter flap top (~#C9A561)
    Color ContentBrowserFolderPrimary   = { 0.627f, 0.482f, 0.267f, 1.0f }; // #A07B44
    Color ContentBrowserFolderEdge      = { 0.361f, 0.275f, 0.157f, 1.0f }; // thin outline (~#5C4728)
    
    // Borders
    Color BorderDefault     = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030
    Color BorderLight       = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030
    Color BorderDark        = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030
    Color BorderSecondary   = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030
    Color Separator         = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030

    // Interactive States
    Color HoverOverlay      = { 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
    Color HoverPanel        = { 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
    Color HoverButton       = { 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
    Color HoverMenu         = { 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
    Color PressedOverlay    = { 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A
    Color SelectedBg        = { 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A

    // Text & Content
    Color TextPrimary       = { 0.835f, 0.835f, 0.835f, 1.0f }; // #D5D5D5 (Menu text)
    Color TextSecondary     = { 0.784f, 0.784f, 0.784f, 1.0f }; // #C8C8C8 (Inactive icons)
    Color TextDisabled      = { 0.478f, 0.478f, 0.478f, 1.0f }; // #7A7A7A (Disabled)
    Color IconMuted         = { 0.580f, 0.608f, 0.651f, 1.0f }; // Slate gray for Lucide icons
    Color TextWindowLabel   = { 0.835f, 0.835f, 0.835f, 1.0f }; // #D5D5D5

    // Accents
    Color SelectedAccent    = { 0.839f, 0.635f, 0.290f, 1.0f }; // #D6A24A
    Color ActiveTabLine     = { 0.839f, 0.635f, 0.290f, 1.0f }; // #D6A24A
    Color Success           = { 0.180f, 0.800f, 0.443f, 1.0f };
    Color Warning           = { 0.945f, 0.768f, 0.058f, 1.0f };

    // Input Fields
    Color InputBackground   = { 0.102f, 0.102f, 0.102f, 1.0f }; // #1A1A1A
    Color SearchBoxBg       = { 0.102f, 0.102f, 0.102f, 1.0f }; // #1A1A1A
    Color SearchPlaceholder = { 0.478f, 0.478f, 0.478f, 1.0f }; // #7A7A7A


    // ------------------------------------------------------------------------
    // Geometry & Layout Constants
    // ------------------------------------------------------------------------
    float CornerRadiusSmall = 4.0f;
    float CornerRadiusMedium= 6.0f;
    float CornerRadiusLarge = 6.0f;
    float WindowCornerRadius = 10.0f;
    
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
    Margin PaddingPanel     = { 8.0f, 8.0f, 8.0f, 8.0f }; 
    Margin PaddingButton    = { 12.0f, 8.0f, 12.0f, 8.0f }; 
    Margin PaddingIconBtn   = { 8.0f, 8.0f, 8.0f, 8.0f };

    // ------------------------------------------------------------------------
    // Global Management
    // ------------------------------------------------------------------------
    static Theme& Get();
};

} // namespace we::editor::application::UI
