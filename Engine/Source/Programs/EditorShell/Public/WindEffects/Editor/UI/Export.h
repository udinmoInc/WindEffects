// ==============================================================================
// WindEffects — EditorShell — Export
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#if defined(_WIN32)
#if defined(EDITORSHELL_EXPORTS)
#define EDITORSHELL_API __declspec(dllexport)
#else
#define EDITORSHELL_API __declspec(dllimport)
#endif
#else
#define EDITORSHELL_API __attribute__((visibility("default")))
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
