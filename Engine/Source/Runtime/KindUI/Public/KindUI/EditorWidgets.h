// ==============================================================================
// WindEffects — KindUI — EditorWidgets
// Single public entry point for shared editor widgets.
//
// Editor code should include this file instead of individual KindUI headers.
// This provides a stable widget API while hiding KindUI implementation details.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

// --- Core widget base and types ---

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/WidgetContext.h"

// --- Icons and theming ---

#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Theming/StyleRole.h"

// --- Layout components ---

#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollViewport.h"
#include "KindUI/Layout/Splitter.h"

// --- Panel system ---

#include "KindUI/Panel/Panel.h"
#include "KindUI/Panel/PanelBuilder.h"
#include "KindUI/Panel/PanelChrome.h"

// --- Shared widgets ---

#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/FilterTabStrip.h"
#include "KindUI/Widgets/CompactTreeWidget.h"
#include "KindUI/Widgets/ScrollContainer.h"
#include "KindUI/Widgets/ObjectTitleBar.h"
#include "KindUI/Core/Widgets/PanelToolbarRow.h"

// --- Editor DSL ---

#include "KindUI/DSL/EditorDSL.h"

// --- Layout helpers ---

#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Rendering/IconMetrics.h"

// --- Input handling ---

#include "KindUI/Input/InputEvents.h"

// --- Editor-specific chrome ---

#include "KindUI/Core/PropertyPanelChrome.h"

namespace we::runtime::kindui {

/// Single using declaration for editor convenience.
/// After #include "KindUI/EditorWidgets.h", use:
///   using namespace we::runtime::kindui;
/// Or use the EditorWidgets namespace alias below.
} // namespace we::runtime::kindui

namespace EditorWidgets = ::we::runtime::kindui;
