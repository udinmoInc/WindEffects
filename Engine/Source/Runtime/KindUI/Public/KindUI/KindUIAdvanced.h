#pragma once

// Advanced KindUI headers for custom widgets, shell integration, and legacy APIs.
// Most applications only need KindUI/KindUI.h.

#include "KindUI/Core/Widget.h"
#include "KindUI/Styles/StyleProperty.h"
#include "KindUI/Styles/WidgetStyles.h"
#include "KindUI/Theming/StyleClass.h"
#include "KindUI/StylePipeline/StylePipeline.h"
#include "KindUI/Animation/StyleAnimationEngine.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Declarative/ViewBuilder.h"

// Legacy token bridge (deprecated — prefer DesignToken + IKindUITheme)
#include "KindUI/Theming/ThemeToken.h"

// Shell / GPU overlay integration (Editor window chrome, not typical app pages)
#include "KindUI/Rendering/OverlayRenderer.h"
