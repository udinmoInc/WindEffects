// ==============================================================================
// WindEffects — Projects — Export
// Public API surface for the Projects module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#if defined(_WIN32)
#if defined(PROJECTS_EXPORTS)
#define PROJECTS_API __declspec(dllexport)
#else
#define PROJECTS_API __declspec(dllimport)
#endif
#else
#define PROJECTS_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
