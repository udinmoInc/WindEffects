#pragma once

// WindEffects Runtime UI SDK ? retained-mode widgets, layout, theming, and overlay rendering.

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Types.h"
#include "WindEffects/Runtime/UI/Core/Widget.h"
#include "WindEffects/Runtime/UI/Core/ApplicationContext.h"
#include "WindEffects/Runtime/UI/Core/WidgetContext.h"
#include "WindEffects/Runtime/UI/Core/Observable.h"
#include "WindEffects/Runtime/UI/Theming/ThemeManager.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"
#include "WindEffects/Runtime/UI/Theming/StyleClass.h"
#include "WindEffects/Runtime/UI/Layout/Box.h"
#include "WindEffects/Runtime/UI/Layout/Flex.h"
#include "WindEffects/Runtime/UI/Layout/Grid.h"
#include "WindEffects/Runtime/UI/Widgets/VirtualList.h"
#include "WindEffects/Runtime/UI/Widgets/Components.h"
#include "WindEffects/Runtime/UI/Rendering/OverlayRenderer.h"

// Preferred namespace for new code. Existing types currently live in WindEffects::Editor::UI
// during the extract-and-evolve migration.
#ifndef WE_RUNTIME_UI_NAMESPACE_ALIAS
#define WE_RUNTIME_UI_NAMESPACE_ALIAS
namespace we::runtime {
namespace ui = ::WindEffects::Editor::UI;
}
#endif
