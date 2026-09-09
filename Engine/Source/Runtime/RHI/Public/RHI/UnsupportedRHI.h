// ==============================================================================
// WindEffects — RHI — UnsupportedRHI
// Public API surface for the RHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "RHI/IRHI.h"

#include <memory>

namespace we::rhi {

// Shared stub IRHI for backends that are not implemented yet (DX11/Metal/GL/GLES).
// Those backends are registered from the RHI module itself — no per-backend stub modules.
[[nodiscard]] RHI_API std::unique_ptr<IRHI> CreateUnsupportedRHI(RHIBackend backend, const char* name);

} // namespace we::rhi
