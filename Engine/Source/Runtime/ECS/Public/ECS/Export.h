// ==============================================================================
// WindEffects — ECS — Export
// Public API surface for the ECS module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#if defined(_WIN32)
#if defined(ECS_EXPORTS)
#define ECS_API __declspec(dllexport)
#else
#define ECS_API __declspec(dllimport)
#endif
#else
#define ECS_API
#endif
