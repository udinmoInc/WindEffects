#pragma once

// KindUI — WindEffects application framework.
// Declarative retained-mode UI: layout, styling, animation, rendering, input, and infrastructure.
//
// Public API entry point. Include this header for the stable SDK surface:
//   #include "KindUI/KindUI.h"
//   using namespace we::runtime::kindui;

#include "KindUI/Export.h"

// Core
#include "KindUI/Core/Types.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/WidgetVariant.h"
#include "KindUI/Core/ApplicationContext.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Core/Observable.h"

// Design tokens & styling
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Styles/StyleProperty.h"
#include "KindUI/Styles/WidgetStyles.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/DefaultTheme.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/StyleClass.h"
#include "KindUI/StylePipeline/StylePipeline.h"
#include "KindUI/Animation/StyleAnimationEngine.h"

// Declarative composition
#include "KindUI/Declarative/Element.h"
#include "KindUI/Declarative/ViewBuilder.h"

// Layout & widgets
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Grid.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/VirtualList.h"

// Rendering
#include "KindUI/Rendering/OverlayRenderer.h"

// Legacy token bridge (deprecated — migrate to DesignToken)
#include "KindUI/Theming/ThemeToken.h"
