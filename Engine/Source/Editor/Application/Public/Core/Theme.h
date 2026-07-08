#pragma once

#include "Application/Export.h"

#include "Core/Geometry.h"
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
    Color ToolbarBackground      = { 0.145f, 0.145f, 0.145f, 1.0f }; // #252525
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
    // Folder icon palette (muted sandstone/tan)
    Color ContentBrowserFolderBody      = { 0.718f, 0.624f, 0.451f, 1.0f }; // #B79F73
    Color ContentBrowserFolderTab       = { 0.776f, 0.690f, 0.510f, 1.0f }; // #C6B082
    Color ContentBrowserFolderShadow    = { 0.541f, 0.463f, 0.333f, 1.0f }; // #8A7655
    Color ContentBrowserFolderHighlight = { 0.847f, 0.780f, 0.608f, 1.0f }; // #D8C79B
    Color ContentBrowserFolderPrimary   = { 0.718f, 0.624f, 0.451f, 1.0f }; // #B79F73
    Color ContentBrowserFolderEdge      = { 0.541f, 0.463f, 0.333f, 1.0f }; // #8A7655
    
    // Borders
    Color BorderLight       = { 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A
    Color BorderDark        = { 0.145f, 0.145f, 0.145f, 1.0f }; // #252525
    Color BorderDefault     = { 0.188f, 0.188f, 0.188f, 1.0f }; // #303030
    Color BorderFocus       = { 0.839f, 0.635f, 0.290f, 1.0f }; // #D6A24A
    Color Separator         = { 0.231f, 0.231f, 0.231f, 0.50f }; // #3B3B3B, 50% opacity

    // Interactive States
    Color HoverBg           = { 0.176f, 0.176f, 0.176f, 1.0f }; // #2D2D2D
    Color ActiveBg          = { 0.290f, 0.290f, 0.290f, 1.0f }; // #4A4A4A
    Color SelectedBg        = { 0.227f, 0.227f, 0.227f, 1.0f }; // #3A3A3A
    Color DisabledBg        = { 0.125f, 0.125f, 0.125f, 1.0f }; // #202020
    
    // Legacy mapping (keep for now so things compile if not all updated yet)
    Color HoverOverlay      = { 0.176f, 0.176f, 0.176f, 1.0f }; 
    Color HoverPanel        = { 0.176f, 0.176f, 0.176f, 1.0f }; 
    Color HoverButton       = { 0.220f, 0.220f, 0.220f, 1.0f }; 
    Color HoverMenu         = { 0.176f, 0.176f, 0.176f, 1.0f }; 
    Color PressedOverlay    = { 0.290f, 0.290f, 0.290f, 1.0f }; 

    // Text & Content
    Color TextPrimary       = { 1.000f, 1.000f, 1.000f, 1.0f }; // #FFFFFF
    Color TextSecondary     = { 0.722f, 0.745f, 0.780f, 1.0f }; // #B8BEC7
    Color TextDisabled      = { 0.478f, 0.478f, 0.478f, 1.0f }; // #7A7A7A
    Color TextMuted         = { 0.550f, 0.550f, 0.550f, 1.0f }; // #8C8C8C
    Color TextWindowLabel   = { 0.835f, 0.835f, 0.835f, 1.0f }; // #D5D5D5

    // Icons (centralized)
    Color IconDefault       = { 0.491f, 0.509f, 0.533f, 1.0f }; // #7D8288 (Dimmed Gray)
    Color IconHover         = { 1.000f, 1.000f, 1.000f, 1.0f }; // #FFFFFF (White)
    Color IconActive        = { 1.000f, 1.000f, 1.000f, 1.0f }; // #FFFFFF
    Color IconDisabled      = { 0.416f, 0.439f, 0.471f, 1.0f }; // #6A7078
    Color IconFolder        = { 0.718f, 0.624f, 0.451f, 1.0f }; // #B79F73 (Colored Folder)
    Color IconMuted         = { 0.580f, 0.608f, 0.651f, 1.0f }; // Legacy

    // Extra backgrounds
    Color ViewportToolbarBg = { 0.100f, 0.100f, 0.100f, 0.85f };
    Color StatusBarBackground = { 0.125f, 0.125f, 0.125f, 1.0f }; // #202020

    Color GizmoAxisX          = { 0.839f, 0.353f, 0.353f, 1.0f }; // #D65A5A
    Color GizmoAxisY          = { 0.404f, 0.725f, 0.435f, 1.0f }; // #67B96F
    Color GizmoAxisZ          = { 0.369f, 0.557f, 0.910f, 1.0f }; // #5E8EE8
    Color GizmoBackground     = { 0.337f, 0.337f, 0.337f, 0.62f }; // #565656

    Color TreeArrow           = { 0.545f, 0.569f, 0.596f, 1.0f }; // #8B9198
    Color SelectionHighlight  = { 0.165f, 0.361f, 0.600f, 1.0f }; // #2A5C99
    Color SearchIcon          = { 0.557f, 0.580f, 0.612f, 1.0f }; // #8E949C

    // Accents
    Color AccentPrimary     = { 0.839f, 0.635f, 0.290f, 1.0f }; // #D6A24A
    Color AccentHover       = { 0.900f, 0.700f, 0.350f, 1.0f };
    Color SelectedAccent    = { 0.839f, 0.635f, 0.290f, 1.0f }; // Legacy
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
