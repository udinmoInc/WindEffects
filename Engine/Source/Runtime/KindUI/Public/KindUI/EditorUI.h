// ==============================================================================
// WindEffects — KindUI — EditorUI
// ONE public include for editor UI. Types live in we::runtime::kindui.
// Declarative panels use we::editor::dsl:
//
//   #include <KindUI/EditorUI.h>
//   using we::editor::dsl::Panel;
//   auto inspector = Panel("Inspector", [](PanelContext& p) {
//       p.Section("Transform", [](auto& s) {
//           s.Property("Location", valueWidget);
//       });
//   });
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

// --- Core ---
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/Expansion.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/UIRepaintGate.h"

// --- Services ---
#include "KindUI/Core/IApplicationContext.h"
#include "KindUI/Core/ApplicationContext.h"
#include "KindUI/Core/ServiceContainer.h"
#include "KindUI/Core/IServiceProvider.h"
#include "KindUI/Theme/IKindUITheme.h"
#include "KindUI/Theme/ThemeManager.h"
#include "KindUI/Theme/GraphiteDarkTheme.h"
#include "KindUI/Core/IResourceRegistry.h"
#include "KindUI/Core/IEventBus.h"
#include "KindUI/Core/ICommandRegistry.h"

// --- Theme ---
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Theme/StyleRole.h"
#include "KindUI/Theme/Palette.h"
#include "KindUI/Theme/PaletteRuntime.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/DesignSystem.h"
#include "KindUI/Theme/SurfaceRole.h"
#include "KindUI/Theme/ChromeSeparation.h"

// --- UI (layout + panels + controls) ---
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Grid.h"
#include "KindUI/UI/Splitter.h"
#include "KindUI/UI/ScrollViewport.h"
#include "KindUI/UI/ScrollLayout.h"
#include "KindUI/UI/CollapsibleGroup.h"
#include "KindUI/UI/PropertyRowLayout.h"
#include "KindUI/UI/OverlayManager.h"
#include "KindUI/UI/IPopupHost.h"
#include "KindUI/UI/AutoAlign.h"

#include "KindUI/UI/Panel.h"
#include "KindUI/UI/PanelBuilder.h"
#include "KindUI/UI/PanelChrome.h"
#include "KindUI/UI/PanelBodyLayout.h"
#include "KindUI/UI/PanelModeTabs.h"

#include "KindUI/UI/Label.h"
#include "KindUI/UI/TextBox.h"
#include "KindUI/UI/CheckBox.h"
#include "KindUI/UI/ColorPicker.h"
#include "KindUI/UI/Components.h"
#include "KindUI/UI/FilterTabStrip.h"
#include "KindUI/UI/CompactTreeWidget.h"
#include "KindUI/UI/ScrollContainer.h"
#include "KindUI/UI/ObjectTitleBar.h"
#include "KindUI/UI/DropdownMenu.h"
#include "KindUI/UI/MenuBar.h"
#include "KindUI/UI/Breadcrumb.h"
#include "KindUI/UI/FormSectionTitle.h"
#include "KindUI/UI/VirtualList.h"
#include "KindUI/UI/ModalHost.h"
#include "KindUI/UI/RichTextView.h"

#include "KindUI/UI/DesignSystemControls.h"
#include "KindUI/UI/PanelToolbarRow.h"
#include "KindUI/UI/ToolbarGlyphButton.h"

// --- Chrome / metrics ---
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/PropertyColumnSplitter.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/UiMetrics.h"
#include "KindUI/Host/IconMetrics.h"

// --- Input ---
#include "KindUI/Core/InputEvents.h"
#include "KindUI/Core/HotkeyManager.h"

// --- Editor composition ---
#include "KindUI/Compose/EditorDSL.h"

// --- Diagnostics (optional heap / memory snapshots for EditorPerf) ---
#include "KindUI/Diagnostics/KindUIHeapStats.h"
