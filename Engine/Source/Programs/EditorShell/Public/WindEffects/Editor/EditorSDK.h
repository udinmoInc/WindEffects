// ==============================================================================
// WindEffects — EditorShell — EditorSDK
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects Editor SDK — single include for editor extension authors.
// Build panels with KindUI widgets; register them through EditorShell services.

#include "WindEffects/Platform.h"
#include "KindUI/KindUI.h"

#include "WindEffects/Editor/UI/EditorShell.h"
#include "WindEffects/Editor/UI/Extensions/ExtensionBootstrap.h"
#include "WindEffects/Editor/UI/Builders/PanelDescriptorBuilder.h"
#include "KindUI/Panel/PanelBuilder.h"
#include "KindUI/Panel/PanelBodyLayout.h"
#include "KindUI/Panel/Panel.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
