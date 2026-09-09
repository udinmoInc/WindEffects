// ==============================================================================
// WindEffects — RHI — Export
// Public API surface for the RHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#if defined(_WIN32)
#if defined(RHI_EXPORTS)
#define RHI_API __declspec(dllexport)
#else
#define RHI_API __declspec(dllimport)
#endif
#else
#define RHI_API __attribute__((visibility("default")))
#endif
