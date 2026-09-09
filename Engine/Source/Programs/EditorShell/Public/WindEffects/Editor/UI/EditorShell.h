// ==============================================================================
// WindEffects — EditorShell — EditorShell
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects Editor Shell — docking, panels, and editor workspace services.
// Widget/layout/rendering foundation lives in Runtime KindUI.
// Extension authors: prefer #include "WindEffects/Editor/EditorSDK.h"

#include "KindUI/KindUI.h"

#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Docking/DockManager.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
#include "WindEffects/Editor/UI/Theming/EditorTheme.h"
