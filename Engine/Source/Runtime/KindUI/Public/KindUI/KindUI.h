#pragma once

// KindUI — WindEffects application framework.
//
// Single public entry point for application development:
//   #include "KindUI/KindUI.h"
//   using namespace we::runtime::kindui;
//   using namespace we::runtime::kindui::UI;
//
// Describe interface structure declaratively; KindUI handles layout, styling,
// rendering, animation, focus, invalidation, and widget lifetime.

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
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/ModalHost.h"
#include "KindUI/Widgets/VirtualList.h"

// --- Advanced / extension (include directly when needed) ----------------------
//
// KindUI/Core/Widget.h              — subclassing & custom widgets
// KindUI/StylePipeline/StylePipeline.h — custom style resolution
// KindUI/Rendering/OverlayRenderer.h — shell GPU integration
// KindUI/Theming/ThemeToken.h       — legacy token bridge (deprecated)
