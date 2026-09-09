// ==============================================================================
// WindEffects — Text — TextSDK
// Public API surface for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects Text SDK — typography, layout, editing, and GPU text rendering.

#include "WindEffects/Platform.h"

#include "Text/TextEngine.h"
#include "Text/Assets/FontAssetManager.h"
#include "Text/Assets/FontResolver.h"
#include "Text/Assets/GlyphResolver.h"
#include "Text/Atlas/FontAtlasManager.h"
#include "Text/Editing/TextEditing.h"
#include "Text/Layout/TextLayoutEngine.h"
#include "Text/Layout/TextStyle.h"
#include "Text/Rendering/TextBatcher.h"
#include "Text/Rich/RichTextParser.h"
#include "Text/Unicode/Grapheme.h"
#include "Text/Unicode/UnicodeDecoder.h"
