// ==============================================================================
// WindEffects — KindUI — KindUI
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// KindUI — WindEffects application framework.
// Single public entry point for application development:
//   #include "KindUI/KindUI.h"
//   using namespace we::runtime::kindui;
//   using namespace we::runtime::kindui::UI;
// HTML-style trees + CSS-style token props (all C++, no .css / WEUI files):
//   auto page = UI::Fill(UI::Bg(
//       }), PaddingToken::Page),
//       ColorToken::PanelBackground));
// Imperative twin on Flex:
//   column->Background(ColorToken::WorkspaceBackground).Padding(PaddingToken::Panel);

#include "KindUI/Export.h"

// --- Core types & state -----------------------------------------------------

#include "KindUI/Core/Types.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/WidgetVariant.h"
#include "KindUI/Core/Observable.h"

// --- Application services ---------------------------------------------------

#include "KindUI/Core/ApplicationContext.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/DefaultTheme.h"
#include "KindUI/Theming/ThemeManager.h"

// --- Declarative programming model (primary DX) -----------------------------

#include "KindUI/Declarative/Element.h"
#include "KindUI/Declarative/UI.h"
#include "KindUI/App/ViewHost.h"
#include "KindUI/App/ApplicationServices.h"
#include "KindUI/App/DialogService.h"
#include "KindUI/App/PopupService.h"

// --- Reusable components & layouts ------------------------------------------

#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Grid.h"
#include "KindUI/Core/Widgets/PanelToolbarRow.h"
#include "KindUI/Core/Widgets/VerticalDivider.h"
#include "KindUI/Core/UiMetrics.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/ModalHost.h"
#include "KindUI/Widgets/VirtualList.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Theming/Palette.h"
#include "KindUI/DSL/EditorDSL.h"

// --- Advanced / extension (include directly when needed) ----------------------
// KindUI/Core/Widget.h              — subclassing & custom widgets
// KindUI/StylePipeline/StylePipeline.h — custom style resolution
// KindUI/Rendering/OverlayRenderer.h — shell GPU integration

// --- Editor-specific widgets (single entry point) ---------------------------
#include "KindUI/EditorUI.h"
